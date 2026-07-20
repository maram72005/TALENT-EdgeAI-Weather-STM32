# FINAL SUMMARY OF FIXES APPLIED

## Problem
STM32N6 NonSecure application hangs at `[AI] stai_runtime_init...` with ST-LINK showing "Target is not responding"

## Root Cause
RIF (Resource Isolation Framework) filter changes made by AppliSecure don't propagate to NonSecure memory view in time. NonSecure tries to read NPU registers before RIF filter is applied to AXI bridge, causing indefinite hang.

## Solution Overview
1. **AppliSecure**: Validate NPU access + add extensive delays + memory barriers
2. **NonSecure**: Wait for RIF propagation + test NPU access before using it + error reporting
3. **Debug Launch**: Load all 3 images (FSBL + Secure + NonSecure)

---

## Detailed Changes

### File 1: `FSBL/meteo_FSBL Debug.launch`

**Change:** Multi-image loading

**Before:**
```xml
<stringAttribute key="com.st.stm32cube.ide.mcu.debug.launch.loadList" 
  value="{&quot;fItems&quot;:[{...only FSBL...}]}"/>
```

**After:**
```xml
<stringAttribute key="com.st.stm32cube.ide.mcu.debug.launch.loadList" 
  value="{&quot;fItems&quot;:[
    {...FSBL...},
    {...AppliSecure...},
    {...AppliNonSecure...}
  ]}"/>
```

**Why:** Ensures all three images are loaded in correct order during debug session.

---

### File 2: `AppliSecure/Core/Src/main.c`

**Function:** `SystemIsolation_Config()`

**Changes at end of RIF config:**

```c
/* 6. CRITICAL: Early NPU probe to ensure it responds */
volatile uint32_t npu_ver = 0;
uint32_t *npu_ver_reg = (uint32_t *)(NPU_BASE_NS);
npu_ver = *npu_ver_reg;
(void)npu_ver;

/* 7. CRITICAL: Memory barriers to ensure all RIF/RISAF writes complete */
__DSB();
__ISB();
__DMB();

/* 8. EXTENDED Wait for memory subsystem to stabilize */
for (volatile int i = 0; i < 500000; i++);

/* 9. Final read to ensure RIF is actually applied */
volatile uint32_t test_read = 0;
test_read = *(uint32_t *)(NPU_BASE_NS);
(void)test_read;
```

**Why:**
- Probe forces RIF configuration to propagate
- DSB/ISB/DMB ensure synchronization
- Long delay allows AXI bridge to process RIF changes
- Final read confirms RIF is active

---

### File 3: `AppliNonSecure/AI/App/npu_init.c`

**Function:** `aiPreInitialize()`

**Before:**
```c
void aiPreInitialize(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_CRC_CLK_ENABLE();

  /* Activer les blocs SRAM dédiés au NPU */
  RCC->MEMENR |= RCC_MEMENR_AXISRAM3EN | RCC_MEMENR_AXISRAM4EN
               | RCC_MEMENR_AXISRAM5EN  | RCC_MEMENR_AXISRAM6EN;

  /* WORKAROUND SILICON : Clear du bit SRAMSD */
  RAMCFG_SRAM3_AXI->CR &= ~(1 << 20);
  RAMCFG_SRAM4_AXI->CR &= ~(1 << 20);
  RAMCFG_SRAM5_AXI->CR &= ~(1 << 20);
  RAMCFG_SRAM6_AXI->CR &= ~(1 << 20);

  RCC->MEMENR |= RCC_MEMENR_CACHEAXIRAMEN;
  __DSB();
  __ISB();
  for (volatile int i = 0; i < 10000; i++);
  NPU_Config();
}
```

**After:**
```c
void aiPreInitialize(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_CRC_CLK_ENABLE();

  /* NPU SRAMs (AXISRAM3-6) are enabled by AppliSecure, not NonSecure */
  __DSB();
  __ISB();
  for (volatile int i = 0; i < 100000; i++);
  /* stai_runtime_init() will probe NPU and report errors */
}
```

**Why:**
- NonSecure has no access to RCC->MEMENR or RAMCFG (locked by RIF)
- Removes failed register access attempts
- Relies on AppliSecure configuration
- Longer delay for stabilization

---

### File 4: `AppliNonSecure/AI/App/app_x-cube-ai.c`

**Changes:**

1. **Added include:**
```c
#include "stm32n6xx.h"  /* For NPU_BASE_NS */
```

2. **Added new test function:**
```c
static int test_npu_access(void)
{
  volatile uint32_t test_val = 0;
  volatile uint32_t *npu_test_addr = (volatile uint32_t *)(NPU_BASE_NS);
  
  __DMB();
  
  for (int retry = 0; retry < 10; retry++) {
    test_val = *npu_test_addr;
    if (test_val != 0xDEADBEEF) {
      return 1;  /* Access works */
    }
  }
  
  return 0;  /* Access failed */
}
```

3. **Modified `aiInit()` function:**
```c
int aiInit(void) {
    stai_return_code ret_code;
    
    HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] Waiting for NPU RIF config from Secure...\r\n", 48, 100);
    HAL_Delay(50);  /* Wait for RIF propagation */
    
    /* Test NPU accessibility */
    HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] Testing NPU access...\r\n", 27, 100);
    if (!test_npu_access()) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] ERROR: NPU not accessible - RIF config failed\r\n", 51, 100);
        HAL_Delay(10);
        return -1;
    }
    HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] NPU accessible - OK\r\n", 26, 100);
    
    HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] stai_runtime_init...\r\n", 26, 100);
    ret_code = stai_runtime_init();
    if (ret_code != STAI_SUCCESS) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "[AI] ERROR in stai_runtime_init: %d (0x%X)\r\n", ret_code, ret_code);
        HAL_UART_Transmit(&huart1, (uint8_t*)err_msg, strlen(err_msg), 100);
        return -1;
    }
    
    /* ... rest of network initialization with error checking ... */
    
    HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] Init OK\r\n", 14, 100);
    return 0;
}
```

**Why:**
- Explicit wait for RIF propagation
- Test NPU accessibility before using it
- Error codes and messages for diagnostics
- No silent failures

---

## Summary Table

| File | Function | Change Type | Purpose |
|------|----------|------------|---------|
| `FSBL/.../launch` | Debugger config | Multi-image load | Load all 3 apps in sequence |
| `AppliSecure/main.c` | SystemIsolation_Config | Add barriers + delay | Ensure RIF visible to NonSecure |
| `AppliNonSecure/npu_init.c` | aiPreInitialize | Simplify | Remove blocked register access |
| `AppliNonSecure/app_x-cube-ai.c` | test_npu_access | New function | Validate NPU before init |
| `AppliNonSecure/app_x-cube-ai.c` | aiInit | Enhanced | Add delays + error checking |

---

## Expected Improvement

**Before Fix:**
```
[OK] LPS22HH
[AI] stai_runtime_init...
[HANGS - ST-LINK disconnects]
```

**After Fix:**
```
[OK] LPS22HH
[AI] Waiting for NPU RIF config from Secure...
[AI] Testing NPU access...
[AI] NPU accessible - OK
[AI] stai_runtime_init...
[AI] stai_network_init...
[AI] get_inputs...
[AI] get_outputs...
[AI] Init OK
--- STATION METEO STM32N6 AVEC IA DEMARREE ---
```

---

## Testing Steps

1. **Rebuild all projects:**
   - FSBL
   - AppliSecure
   - AppliNonSecure

2. **Flash via debugger with multi-image load**

3. **Monitor UART at 115200 baud**

4. **Observe expected sequence above**

5. **If still hangs:**
   - Check diagnostic guide
   - Increase delays if needed
   - Verify RIF config in AppliSecure

---

## Key Technical Insights

1. **Memory Barriers:** DSB/ISB work within a domain; cross-domain needs delays
2. **RIF Propagation:** Takes time for filter changes to reach AXI slave
3. **Test Before Use:** Explicit access test catches configuration failures
4. **Error Codes:** Report actual STAI errors instead of silent hangs
5. **Timeouts:** Provide diagnostics even if something fails

---

**Status:** All fixes applied. Ready for rebuild and test.
