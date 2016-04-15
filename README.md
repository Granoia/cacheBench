# cacheBench

The jsmn library can be installed with git clone https://github.com/zserge/jsmn and the included makefile.


<h1> System Definition</h1>
The goal of the cache is to set and return key,value pairs of a network.
Cache behavior is correct if a deleted value is never returned, and a get returns the most recently set value with that key or nil.
Optimistically, a set value should be successfully returned if no other sets have occurred (i.e. eviction has not occurred).
Due to the limitations of the slab allocator, we consider it correct behavior if a value is not set provided an informative error is returned (eg. "Value too large for any slabs").

<h1> Services and Outcomes</h1>
<b>GET /k </b> - Returns the correct value or nil. Returns an error message if the value is too large.

<b>SET /k/v </b> - Sets the value and returns a success message.

<b>DELETE /k/v </b> - Deletes the value and returns a success message.

<b>PUT /memsize/v </b> - If no cache exists, initiates a new cache with the given max memory.

<h1>Metric</h1>
<b>Sustained throughput (rqsts/sec):</b> Maximum load at which mean response time remains under 1 ms

<h1>Parameters</h1>

<h1>Factors</h1>

<h1>Evaluation Technique</h1>

<h1>Workload</h1>
The Memcache ETC workload:

We used the following distribution for size of set values:
40%   2, 3, and 11 bytes
3%    3-10 bytes
7%    12-100 bytes
45%   100-500 bytes
4.8%  500-1000 bytes
.2%   1000bytes-1MB


</h1>Experiment</h1>

<h1>Analysis</h1>

<h1>Results</h1>
