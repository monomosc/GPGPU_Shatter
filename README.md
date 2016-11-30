# GPGPU_Shatter

# 1 Defined Functions
# 2 Functionality
# 3 Recommended Ordering
# 4 Argument and Ordering Constraints


# 1 Defined Functions

# The following functions are defined by IsShattered
# void Initialize()
# void PassVisibilityMatrix(byte*, INT64)
# INT64 DispatchCheck(INT64)
# void Compute()
# INT64 result()
# INT64 GetIndicesByIndex(INT64)
# void Cleanup()


# 2 Functionality

# Initialize() and Cleanup() are to be called at the start and termination of program respectively
# PassVisibilityMatrix(byte*, INT64) will set the GPU Buffer for a 1D array of bools. The first argument bytearray contains 8bit 
# boolean values 0x01 or 0x00. The second argument denotes the vertex amount, not the size of the bytearray (which is the square)
# DispatchCheck will prepare an Index Array of up to 6 byte (the first argument) for calculation.
# Compute() will dispatch the calculation - it is recommended Compute is only called after exactly 1024 calls of DispatchCheck
# have been executed. After Compute() has been called it is recommended to perform additional 1024 calls of DispatchCheck()
# before calling result()
# result() will not return until the previous asynchronous Dispatch() (The D3D11 API call) has been completed.
# The return value of result is either negative for a Code, or positive for a Index of an Index array that has been found shattered
# and can be returned by GetIndicesByIndex(INT64). result() returns -2 on a calculation Error (bug); returns -1 on no Shattered Set
# found
# INT64 GetIndicesByIndex(INT64) returns the indices array in a 8byte INT64. This should as of now, be called even if result()
# returns an error/no success code. (To be fixed)

# 3 Recommended Ordering

# A program should call functions in the following order
# Initialize
# PassVisibilityMatrix()
# DispatchCheck()  until it returns 1
# loop
#  Compute
#  DispatchCheck() until it returns 1
#  result()
#  GetIndicesByIndex()
# Cleanup

# 4 Argument and Ordering Constraints

# It is responsibility of the caller to check arguments for sanity
# PassVisibilityMatrix(byte* a, INT64 b)
# Ensure that length of a == b²
# Calling PassVisibilityMatrix again is permitted, assuming all previous Compute() calls have been finished with result() and
# GetIndicesByIndex()

# DispatchCheck(INT64 a)
# a HAS TO BE IN ASCENDING ORDER! Unused Bytes need to be set to zero. Bytes 7 & 8 need to be Zero.
# DispatchCheck(0x0000000000000000) causes undefined behaviour (to be fixed)

# Compute()
# This currently can only be called safely if exactly MAX_QUEUE_SIZE calls to DispatchCheck(INT64 a) have been made (to be fixed)
# As a temporary Workaround, pass sets that definitely are Not Shattered.
# Note that calling Compute() while Computation is already in progress will cause Undefined Behaviour or crashes.
# Before calling Compute() again always call result() in order to wait for calculation to finish.

# result()
# Never call without Computation in progress. Will cause crashes else.
# The return value is a single number without meaning outside of this API  (Index indexing indices)
# or an error code.

# GetIndicesByIndex(INT64)
# Only call this with non-negative return values of result(). ALways call right after Compute() for Memory-Leak Reasons



