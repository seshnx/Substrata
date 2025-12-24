# Debugging Plugin Crash on Load

If the plugin is still crashing, try these steps to isolate the issue:

## Test 1: Minimal Editor
Temporarily simplify the editor constructor to see if it's an initialization issue:

```cpp
SubstrataAudioProcessorEditor::SubstrataAudioProcessorEditor(SubstrataAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(800, 600);
    // Comment out all other initialization temporarily
}
```

## Test 2: Check Standalone vs Plugin
- Does standalone crash immediately or after opening?
- Does it crash in the DAW on plugin load or when opening the editor?

## Common Crash Causes:
1. Parameter initialization issues
2. Editor accessing processor before ready
3. ComboBox attachment issues
4. Timer starting before component ready
5. Uninitialized DSP components

## Check Windows Event Viewer
Look for access violations or exceptions that might give more details about what's crashing.

