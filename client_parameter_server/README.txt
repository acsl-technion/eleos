1. This is a client for testing the parameter server micro-benchmark
It generates Random requests in a given range, requests are uint64

 Usage ./a.out [REQUEST_NUM] [USE_FILE] [File_PATH] [CACHE_SIZE(MB)]
 REQUEST_NUM = Number of requests generated to the parameter server (in bytes)
 USE_FILE = Use a file of pregenerated encrypted requests (1/0)
 FILE_PATH = The encrpyted requests File_path (string)
 CACHE_SIZE(MB) = Generate requests between 0-$_size.
