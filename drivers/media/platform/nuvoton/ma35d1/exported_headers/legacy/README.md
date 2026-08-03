# VC8000 Legacy Core ABI

These headers are the transitional declaration boundary between the open
Linux shim and the prebuilt VC8000 core. Customer Kbuild uses this directory
and does not require the proprietary `source/` tree.

The files intentionally preserve the legacy VC8000 data structures and entry
points used by the current shim. Do not add Linux headers or Linux-only types.
New interfaces belong in `vc_core_abi.h` and `vc_os.h`; the long-term goal is
to retire this legacy set as the versioned core ABI is adopted.
