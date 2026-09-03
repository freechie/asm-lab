import lldb

# Loaded by count_insns.py via `command script import`. Not a lesson driver.


def count_until_return(debugger, command, result, internal_dict):
    """Step from the current frame until it returns. Print the instruction count.

    This is the honest 'how many machine instructions did a+b take' number.
    It includes every callee of PyNumber_Add. A C `add w0, w0, w1` is one.
    """
    target = debugger.GetSelectedTarget()
    process = target.GetProcess()
    thread = process.GetSelectedThread()
    start = thread.GetNumFrames()
    n = 0
    cap = 200000
    while n < cap:
        thread.StepInstruction(False)
        n += 1
        if thread.GetNumFrames() < start:
            break
        if process.GetState() == lldb.eStateExited:
            break
    result.AppendMessage(f"INSTRUCTIONS {n}")
    if n >= cap:
        result.AppendMessage("CAP_HIT")


def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand(
        "command script add -f count_until_return.count_until_return "
        "count_until_return"
    )
