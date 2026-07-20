# NPU Initialization Troubleshooting Guide

## Current Status
The application successfully initializes sensors but hangs during AI initialization at or before `stai_runtime_init()`.

## Expected Output Sequence (After Fix)

```
[HTS221] Initialise OK
[OK] HTS221
[LPS22HH] Initialise OK (10Hz)
[OK] LPS22HH

[DEBUG] Tentative init IA...
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

## Diagnostic Steps

### Step 1: Check if NonSecure AI Init Starts
**What to look for:** `[DEBUG] Tentative init IA...`

**If MISSING:**
- Problem is in NonSecure boot sequence
- Check AppliNonSecure/Core/Src/main.c
- Verify sensors initialization completes

**If PRESENT:** → Proceed to Step 2

---

### Step 2: Check if RIF Wait Completes
**What to look for:** `[AI] Waiting for NPU RIF config from Secure...`

**If MISSING or HANGS:**
- Problem is in STM32CubeAI_Studio_AI_Init() call
- Check if aiPreInitialize() hangs
- Verify AppliNonSecure/AI/App/npu_init.c is correct

**If PRESENT:** → Proceed to Step 3

---

### Step 3: Check NPU Accessibility Test
**What to look for:** `[AI] Testing NPU access...`

**If MISSING or HANGS:**
- Problem: 50ms delay might not be enough
- Solution: Increase delay in aiInit() to 100ms or 200ms

**If shows ERROR message:**
```
[AI] ERROR: NPU not accessible - RIF config failed
```
This means:
1. AppliSecure didn't properly open RIF filter for NPU
2. OR the early NPU probe in Secure failed
3. OR memory barriers didn't work properly

**Actions:**
- Check AppliSecure/Core/Src/main.c - verify NPU RIF config is present
- Verify RISAF4-7 registers are set to 0x00000000
- Check that RIMC_master config for NPU is NonSecure

**If shows OK:** → Proceed to Step 4

---

### Step 4: Check stai_runtime_init() Execution
**What to look for:** `[AI] stai_runtime_init...`

**If MISSING or HANGS:**
- Problem: NPU registers don't respond to read attempts
- Likely causes:
  1. NPU power domain not enabled
  2. AXISRAM3-6 clocks not enabled in Secure
  3. RAMCFG shutdown bits not cleared

**If shows ERROR with code:**
```
[AI] ERROR in stai_runtime_init: N (0xX)
```
Report this code number - it indicates specific STAI failure

**If successful:** → Proceed to Step 5

---

### Step 5: Check Network Initialization
**What to look for:** `[AI] stai_network_init...`

**If ERROR:**
- Network model might be corrupted
- Check ai_generated_network/ folder contents
- Verify network weights are loaded correctly

---

## Quick Fix Checklist

### For "NPU not accessible" Error:

1. **Verify AppliSecure RIF Configuration:**
   ```c
   /* In AppliSecure/Core/Src/main.c, SystemIsolation_Config() */
   RISAF4->REG[0].CFGR = 0x00000000;  /* AXISRAM3 */
   RISAF5->REG[0].CFGR = 0x00000000;  /* AXISRAM4 */
   RISAF6->REG[0].CFGR = 0x00000000;  /* AXISRAM5 */
   RISAF7->REG[0].CFGR = 0x00000000;  /* AXISRAM6 */
   ```

2. **Verify NPU Clock Enabled:**
   ```c
   RCC->MEMENR |= RCC_MEMENR_AXISRAM3EN | RCC_MEMENR_AXISRAM4EN
                | RCC_MEMENR_AXISRAM5EN  | RCC_MEMENR_AXISRAM6EN;
   ```

3. **Verify Secure Has Time to Configure:**
   - Increase delay in AppliSecure from 500K to 1000K cycles
   - Increase delay in NonSecure from 50ms to 100ms

4. **Verify FSBL Enables NPU:**
   - Check FSBL/Core/Src/main.c or periph_init.c
   - Ensure NPU clocks are enabled before jumping to Secure

### For stai_runtime_init() Hanging:

1. **Increase delay after NPU access test:**
   ```c
   HAL_Delay(100);  /* Increase from 50ms to 100ms */
   ```

2. **Add watchdog to prevent infinite hang:**
   ```c
   uint32_t start_time = HAL_GetTick();
   ret_code = stai_runtime_init();
   uint32_t elapsed = HAL_GetTick() - start_time;
   if (elapsed > 5000) {
       snprintf(err_msg, sizeof(err_msg), "[AI] WARNING: stai_runtime_init took %ums\r\n", elapsed);
       HAL_UART_Transmit(&huart1, (uint8_t*)err_msg, strlen(err_msg), 100);
   }
   ```

3. **Check if problem is silicon-specific:**
   - Some revisions of STM32N6 may have RIF propagation delays
   - Try exponentially longer delays until it works

---

## Debug Output Capture

To help diagnose, please provide complete UART output showing:
1. Which step completes without "ERROR"
2. At what point it hangs or shows error
3. Any error codes displayed

Example format:
```
[Sensors init messages...]
[DEBUG] Tentative init IA...
[AI] Waiting for NPU RIF config from Secure...
[AI] Testing NPU access...
[AI] ERROR: NPU not accessible - RIF config failed  ← STOPS HERE
```

---

## Next Steps If Still Not Working

1. **Contact ST Support with:**
   - Board revision (STM32N657X0HXQ)
   - CubeIDE version
   - Complete UART output
   - Any timing constraints or clocking peculiarities

2. **Try Workaround:**
   - Disable AI features temporarily
   - Run sensors/display only to verify boot chain works
   - Once confirmed, gradually enable AI initialization

3. **Check Hardware:**
   - Verify all SWD connections are solid
   - Check power supply is stable
   - Try different ST-LINK programmer if available
