import os

CHECK_PTY_FLAG_VAR = "VASTCLOUD_CHECK_PTY"
CHECK_PTY = CHECK_PTY_FLAG_VAR in os.environ

NO_PTY_FLAG_VAR = "VASTCLOUD_NO_PTY"
NO_PTY = NO_PTY_FLAG_VAR in os.environ

TRACE_FLAG_VAR = "VASTCLOUD_TRACE"
TRACE = TRACE_FLAG_VAR in os.environ
