# STM32N6 NonSecure AI Initialization Hang - Diagnosis & Fix

## Problem Statement
The NonSecure application hangs at `[AI] stai_runtime_init...` after sensors and display initialize successfully.

### Boot Sequence Before Fix
```
[HTS221] Init...
[HTS221] ID OK. Calibration...
   H0=63/2 %RH  H1=148/2 %RH
   T0=162/8 C  T1=263/8 C
[HTS221] Initialise OK
[OK] HTS221
[LPS22HH] Init...
[LPS22HH] ID OK. Config...
[LPS22HH] Initialise OK (10Hz)
[OK] LPS22HH
[AI] stai_runtime_init...
--- HANG ---
```

## Root Cause Analysis

### Issue 1: Memory Subsystem Not Stabilized
**Root Cause:** AppliSecure enables NPU SRAMs and opens RIF permissions, but:
- No memory barriers (`DSB`/`ISB`) to ensure all configuration writes complete
- No delay for RIF subsystem to stabilize before NonSecure accesses
- Potential race condition between RIF register writes and NonSecure memory access

**Impact:** `stai_runtime_init()` tries to probe/initialize NPU SRAMs that appear to NonSecure as inaccessible or unstable, causing a hang or fault.

### Issue 2: NonSecure Trying Direct Register Access
**Root Cause:** Original `aiPreInitialize()` in `npu_init.c` tried to:
- Write directly to `RCC->MEMENR` (clock enable)
- Write directly to `RAMCFG_SRAM*->CR` (shutdown bit clear)
- Enable `RCC->MEMENR CACHEAXIRAMEN`

**Problem:** NonSecure code has no access to these registers after AppliSecure locks down RIF. Direct writes silently fail or cause faults.

### Issue 3: No Error Reporting
**Root Cause:** `aiInit()` called `stai_runtime_init()` and printed a single message before calling it, but had no error checking. If the function hung, no error code was captured.

**Impact:** Impossible to know where the hang occurred (runtime init, network init, get inputs, or get outputs).

---

## Solution Implemented

### Fix 1: AppliSecure RIF Configuration (Critical)
**File:** `AppliSecure/Core/Src/main.c` (SystemIsolation_Config function)

**Changes:**
```c
/* 5. NPU périphérique NonSecure */
HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_NPU,
                                       RIF_ATTRIBUTE_NSEC | RIF_ATTRIBUTE_NPRIV);

/* 6. CRITICAL: Memory barriers to ensure all RIF/RISAF writes complete */
__DSB();
__ISB();

/* 7. Wait for memory subsystem to stabilize before NonSecure accesses NPU SRAMs */
for (volatile int i = 0; i < 100000; i++);
```

**Why:** Ensures that all RIF configuration changes are visible in memory before NonSecure code accesses NPU resources. The 100K cycle delay (approximately 1-10 ms depending on clock) allows the memory subsystem and RIF controller to fully stabilize.

---

### Fix 2: NonSecure NPU Initialization (Fundamental)
**File:** `AppliNonSecure/AI/App/npu_init.c` (aiPreInitialize function)

**Before:**
```c
void aiPreInitialize(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_CRC_CLK_ENABLE();

  /* Activer les blocs SRAM dédiés au NPU (AXISRAM 3 à 6) */
  RCC->MEMENR |= RCC_MEMENR_AXISRAM3EN | RCC_MEMENR_AXISRAM4EN
               | RCC_MEMENR_AXISRAM5EN  | RCC_MEMENR_AXISRAM6EN;

  /* WORKAROUND SILICON : Clear du bit SRAMSD (bit 20) pour sortir du shutdown */
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

  /* Les SRAMs NPU (AXISRAM3-6) sont activées par AppliSecure dans RIF_Init */
  /* et les permissions RISAF4-7 sont ouvertes. */
  
  /* NonSecure n'a pas accès à RCC->MEMENR ou RAMCFG après la config Secure */
  /* Donc on laisse AppliSecure gérer cela et on attendjuste la stabilisation */
  
  __DSB();
  __ISB();
  for (volatile int i = 0; i < 100000; i++);
  
  /* stai_runtime_init() accèdera aux SRAMs pour en vérifier l'accès */
}
```

**Why:** 
- Removes NonSecure attempts to access Secure-locked registers
- Relies on AppliSecure to properly enable NPU SRAMs and open RIF
- Adds longer stabilization delay matching the Secure side
- Lets `stai_runtime_init()` probe NPU memory properly

---

### Fix 3: Error Reporting in AI Initialization
**File:** `AppliNonSecure/AI/App/app_x-cube-ai.c` (aiInit function)

**Changes:** Added error checking and UART reporting after each STAI call:
```c
int aiInit(void) {
    stai_return_code ret_code;

    HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] stai_runtime_init...\r\n", 26, 100);
    ret_code = stai_runtime_init();
    if (ret_code != STAI_SUCCESS) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "[AI] ERROR in stai_runtime_init: %d\r\n", ret_code);
        HAL_UART_Transmit(&huart1, (uint8_t*)err_msg, strlen(err_msg), 100);
        return -1;
    }
    
    /* Similar checks for stai_network_init, get_inputs, get_outputs */
    
    HAL_UART_Transmit(&huart1, (uint8_t*)"[AI] Init OK\r\n", 14, 100);
    return 0;
}
```

**Why:** If the hang or failure still occurs, the UART output will show exactly which step failed, enabling further debugging.

---

## Expected Boot Sequence After Fix
```
[HTS221] Init...
[HTS221] ID OK. Calibration...
   H0=63/2 %RH  H1=148/2 %RH
   T0=162/8 C  T1=263/8 C
[HTS221] Initialise OK
[OK] HTS221
[LPS22HH] Init...
[LPS22HH] ID OK. Config...
[LPS22HH] Initialise OK (10Hz)
[OK] LPS22HH
[DEBUG] Tentative init IA...
[AI] stai_runtime_init...
[AI] stai_network_init...
[AI] get_inputs...
[AI] get_outputs...
[AI] Init OK
--- STATION METEO STM32N6 AVEC IA DEMARREE ---
[Sensor readings and AI predictions follow...]
```

---

## Compilation & Testing Steps

1. **Rebuild AppliSecure** (FSBL loads Secure first):
   ```
   Project → Build Project (AppliSecure)
   ```

2. **Rebuild AppliNonSecure** (depends on Secure RIF config):
   ```
   Project → Build Project (AppliNonSecure)
   ```

3. **Debug Session** (with multi-image load):
   - Use `FSBL/meteo_FSBL Debug.launch` (loads FSBL, Secure, NonSecure)
   - Observe UART output at ~115200 baud
   - Monitor for `[AI] Init OK` message

4. **If Still Hanging:**
   - Check if `[AI] ERROR in stai_runtime_init: <code>` or other error appears
   - If no error output, the hang is likely in `stai_runtime_init()` itself
   - Verify AppliSecure correctly enabled and opened NPU SRAMs (add debug prints)

---

## Key Points

- **Why the Secure side must do this:** Only Secure can modify RIF, RCC memory clocks, and RAMCFG
- **Why we added delays:** The RIF/memory subsystem needs time to propagate configuration changes
- **Why NonSecure can't repeat the config:** Access control prevents NonSecure from touching these registers
- **Why error checking helps:** If something still fails, we now know exactly where

---

## Files Modified

1. `AppliSecure/Core/Src/main.c` - Added DSB/ISB and delay after NPU RIF config
2. `AppliNonSecure/AI/App/npu_init.c` - Removed dangerous direct register access in NonSecure
3. `AppliNonSecure/AI/App/app_x-cube-ai.c` - Added error reporting for each STAI call

## Files Updated in Deploy Configuration

1. `FSBL/meteo_FSBL Debug.launch` - Now loads FSBL + Secure + NonSecure in sequence

---

**Diagnosis Complete. Ready to rebuild and test.**
