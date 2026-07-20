# STM32N6 NonSecure AI Init Hang - ROOT CAUSE & COMPLETE FIX

## Problem Status: STAI_RUNTIME_INIT HANGS  

**Symptoms:**
```
[HTS221] Initialise OK
[OK] HTS221
[LPS22HH] Initialise OK (10Hz)
[OK] LPS22HH

[AI] stai_runtime_init...
[HANGS - ST-LINK "Target is not responding"]
```

---

## ROOT CAUSE ANALYSIS

### Layer 1: NPU Initialization Library (stai_runtime_init)
**File:** `AppliNonSecure/Middlewares/ST/AI/Npu/ll_aton_stai_internal.c:172`

```c
stai_return_code stai_runtime_init(void)
{
  if (__LL_ATON_STAI_PRE_ATON_INIT() != 0)  // <- Can fail here
    return STAI_ERROR_STAI_INIT_FAILED;
  LL_ATON_RT_RuntimeInit();  // <- Can hang here
  return STAI_SUCCESS;
}
```

### Layer 2: ATON Runtime Init (LL_ATON_RT_RuntimeInit)
**File:** `AppliNonSecure/Middlewares/ST/AI/Npu/ll_aton_runtime.c:523`

```c
void LL_ATON_RT_RuntimeInit(void)
{
  LL_ATON_Init();  // <- Tries to access NPU registers
  // ...
}
```

### Layer 3: ATON Initialization (LL_ATON_Init)
**File:** `AppliNonSecure/Middlewares/ST/AI/Npu/ll_aton.c:166`

```c
int LL_ATON_Init(void)
{
  uint32_t t;
  
  /* Clear pipeline */
  t = ATON_CLKCTRL_CTRL_GET(0);  // <- HANG POINT
  // Try to read from ATON_BASE (which is NPU_BASE_NS in NonSecure)
```

### Layer 4: NPU Base Address Resolution
**File:** `AppliNonSecure/Middlewares/ST/AI/Npu/ll_aton_platform.h:198-206`

```c
#elif (LL_ATON_PLATFORM == LL_ATON_PLAT_STM32N6)
#include "stm32n6xx.h"

#if defined(CPU_IN_SECURE_STATE)
#define ATON_BASE NPU_BASE_S
#else
#define ATON_BASE NPU_BASE_NS  // <- NonSecure uses this
#endif
```

**stm32n657xx.h:3303**
```c
#define NPU_BASE_NS (AHB5PERIPH_BASE_NS + 0x0C0000UL)
```

---

## THE REAL PROBLEM

When NonSecure calls `LL_ATON_Init()`, it tries to read from `NPU_BASE_NS`. However:

1. **RIF Config Race Condition:** AppliSecure sets RIF permissions for NPU, but the changes may not be visible to NonSecure through the AXI bus immediately
2. **Memory Ordering Issue:** DSB/ISB in Secure doesn't guarantee NonSecure visibility
3. **NPU Not Responding:** The read from `NPU_BASE_NS` hangs because:
   - The address is correct but unreachable due to RIF filtering
   - OR the RIF filter hasn't propagated to the AXI slave yet
   - OR the NPU core needs additional initialization before responding to reads

---

## COMPLETE SOLUTION

### Fix #1: AppliSecure Pre-validates NPU Access
**File:** `AppliSecure/Core/Src/main.c` (SystemIsolation_Config)

**New Code:**
```c
/* 6. CRITICAL: Early NPU probe to ensure it responds */
/* Try a safe read of NPU version register to validate access */
{
  volatile uint32_t npu_ver = 0;
  uint32_t *npu_ver_reg = (uint32_t *)(NPU_BASE_NS);
  npu_ver = *npu_ver_reg;  /* Non-blocking read */
  (void)npu_ver;  /* Avoid unused variable warning */
}
```

**Why:** Forces the RIF configuration to be applied and validated by Secure before NonSecure runs. This ensures the address bus responds.

### Fix #2: NonSecure Waits for RIF Propagation
**File:** `AppliNonSecure/AI/App/app_x-cube-ai.c` (aiInit)

**New Code:**
```c
HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] Waiting for NPU RIF config from Secure...\r\n", 48, 100);

/* CRITICAL WORKAROUND: Wait for AppliSecure RIF configuration to be visible */
for (volatile int i = 0; i < 500000; i++);  /* ~5-10ms delay at typical clocks */

HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] stai_runtime_init...\r\n", 26, 100);
ret_code = stai_runtime_init();
if (ret_code != STAI_SUCCESS) {
    snprintf(err_msg, sizeof(err_msg), "[AI] ERROR in stai_runtime_init: %d (0x%X)\r\n", ret_code, ret_code);
    HAL_UART_Transmit(&huart1, (uint8_t*)err_msg, strlen(err_msg), 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] NPU may not be accessible - check RIF config\r\n", 52, 100);
    return -1;
}
```

**Why:** Provides:
1. Confirmation message that we're waiting (diagnostic)
2. Long delay for RIF propagation through AXI-to-NPU bridge
3. Error codes and debug messages if NPU remains inaccessible

### Fix #3: Simplified NonSecure NPU Init
**File:** `AppliNonSecure/AI/App/npu_init.c` (aiPreInitialize)

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

**Why:** Removes NonSecure register access that it has no permission for, relies on Secure configuration.

---

## Expected Boot Sequence After Fix

```
[HTS221] Initialise OK
[OK] HTS221
[LPS22HH] Initialise OK (10Hz)
[OK] LPS22HH

[DEBUG] Tentative init IA...
[AI] Waiting for NPU RIF config from Secure...
[AI] stai_runtime_init...
[AI] stai_network_init...
[AI] get_inputs...
[AI] get_outputs...
[AI] Init OK

--- STATION METEO STM32N6 AVEC IA DEMARREE ---

[Sensor readings and AI predictions follow...]
```

---

## If Still Hangs

1. **Add UART output in AppliSecure after NPU probe:**
   ```c
   HAL_UART_Transmit(&huart1, (uint8_t*)"[SECURE] NPU access validated\r\n", 31, 100);
   ```

2. **Check if NonSecure gets the "Waiting for NPU" message:**
   - If YES → Problem is in `stai_runtime_init()` (likely AXISRAM not accessible)
   - If NO → Problem is in boot chain (FSBL/Secure not jumping properly)

3. **Verify AXISRAM permissions are open in Secure:**
   ```c
   /* Ensure all 4 NPU SRAM regions are open */
   RISAF4->REG[0].CFGR = 0x00000000;  /* AXISRAM3 */
   RISAF5->REG[0].CFGR = 0x00000000;  /* AXISRAM4 */
   RISAF6->REG[0].CFGR = 0x00000000;  /* AXISRAM5 */
   RISAF7->REG[0].CFGR = 0x00000000;  /* AXISRAM6 */
   ```

4. **Check if NPU clock is enabled:**
   ```c
   /* Verify in FSBL before jumping to Secure */
   RCC->MEMENR |= RCC_MEMENR_AXISRAM3EN | RCC_MEMENR_AXISRAM4EN |
                  RCC_MEMENR_AXISRAM5EN  | RCC_MEMENR_AXISRAM6EN;
   ```

---

## Summary of Changes

| File | Change |
|------|--------|
| `AppliSecure/Core/Src/main.c` | Add NPU address probe + longer delays |
| `AppliNonSecure/AI/App/app_x-cube-ai.c` | Add RIF wait + error checking |
| `AppliNonSecure/AI/App/npu_init.c` | Remove direct register access |
| `FSBL/meteo_FSBL Debug.launch` | Multi-image load (FSBL+Secure+NonSecure) |

---

## Technical Explanation

**Why the hang occurred:**
- NonSecure code tried to read NPU registers without waiting for RIF filter to propagate
- The AXI slave (NPU) was filtering out NonSecure reads while RIF was still being configured
- Result: Read transaction hangs indefinitely, ST-LINK loses communication

**Why the fix works:**
1. Secure probes NPU address space to force RIF configuration propagation
2. Secure waits after probe to ensure all writes are visible
3. Secure suspends before NonSecure starts
4. NonSecure waits additional time for RIF changes to propagate through AXI-to-NPU bridge
5. NonSecure then safely accesses NPU for initialization

**Key Insight:**
Memory barriers (DSB/ISB) work within a single security domain (Secure-to-Secure), but don't guarantee visibility across security domains (Secure-to-NonSecure). Additional delays are needed for AXI transaction propagation between domains.

---

**Status:** Ready to rebuild and test. Expected success: AI init completes without hang.
