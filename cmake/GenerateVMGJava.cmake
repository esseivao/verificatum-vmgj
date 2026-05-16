if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE is required")
endif()

if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE is required")
endif()

if(NOT DEFINED VMGJ_VERSION)
    message(FATAL_ERROR "VMGJ_VERSION is required")
endif()

file(READ "${INPUT_FILE}" _contents)
string(REPLACE "M4_VERSION" "${VMGJ_VERSION}" _contents "${_contents}")
string(REPLACE "VMGJ_VERSION_STRING" "${VMGJ_VERSION}" _contents "${_contents}")
file(WRITE "${OUTPUT_FILE}" "${_contents}")
