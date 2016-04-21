# cacheBench

The jsmn library can be installed with git clone https://github.com/zserge/jsmn and the included makefile.

We had to change our implementation to run the tests. We have been using the "REQREP" socket protocol, but it doesn't allow a socket to receive twice in a row (which wouldn't be a request-reply protocol). Since we want to send a bunch of requests and then receive a bunch of replies, we changed the protocol to PAIR. We don't know how much this affects our numbers.

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
Speed of the server CPU--
Speed of the client CPU--
Speed of the network--
Operating system overhead
Reliability of the network

<h3>Workload Parameters</h3>
Time between requests
Size of keys
Size of values
Distribution of keys (for hashing)
Other loads on the CPU  -- should be minimal
Other loads on the network -- should be minimal


<h1>Factors</h1>
The only factor to be varied is the rate of requests from the client. 


<h1>Evaluation Technique</h1>
A prototype server has been implemented, but some specificities of the implementation make direct measurement difficult. A modified server was designed to process requests in the same way as the prototype, but handle networking via a different protocol. Specifically, the modified server uses the one-to-one "PAIR" protocol, and handles requests and replies without blocking. The working server uses the many-to-one "REQ-REP" protocol, which necessitates blocking. The performance difference should only be on the client side, because the server replies immediately to requests in any case; however performance differences between the protocols are not known.

<h1>Workload</h1>
The Memcache ETC workload:

We used the following distribution for size of set values:
40%   2, 3, and 11 bytes
3%    3-10 bytes
7%    12-100 bytes
45%   100-500 bytes
4.8%  500-1000 bytes
.2%   1000bytes-1MB


<h1>Experiment</h1>
The workload simulator was run for 30 seconds during which it sent requests according to the workload distribution. Requests were sent with a uniform temporal distribution. The only independent parameter was average time between requests (measured in microseconds). The discrepancy between messages sent and received was recorded as a measure of throughput, but in practice the client simply froze when the throughput dropped.
The simulator was run five times per request-delay. If no freezes occurred, the delay was decreased. The lowest mesage delay without any freezes was used to calculate the maximum throughput.

<h1>Results</h1>

<table class="tg" style="undefined;table-layout: fixed; width: 352px">
<colgroup>
<col style="width: 192px">
<col style="width: 160px">
</colgroup>
  <tr>
    <th class="tg-yw4l">Average Request Frequency (requests/sec)</th>
    <th class="tg-yw4l">Jammed?</th>
  </tr>
  <tr>
    <td class="tg-yw4l">2000</td>
    <td class="tg-yw4l">no</td>
  </tr>
  <tr>
    <td class="tg-yw4l">4000</td>
    <td class="tg-yw4l">no</td>
  </tr>
  <tr>
    <td class="tg-yw4l">4444</td>
    <td class="tg-yw4l">no</td>
  </tr>
  <tr>
    <td class="tg-yw4l">5000</td>
    <td class="tg-yw4l">no</td>
  </tr>
  <tr>
    <td class="tg-yw4l">5714</td>
    <td class="tg-yw4l">no</td>
  </tr>
  <tr>
    <td class="tg-yw4l">6666</td>
    <td class="tg-yw4l">no</td>
  </tr>
  <tr>
    <td class="tg-yw4l">8000</td>
    <td class="tg-yw4l">no</td>
  </tr>
  <tr>
    <td class="tg-yw4l">8695</td>
    <td class="tg-yw4l">no</td>
  </tr>
  <tr>
    <td class="tg-yw4l">9090</td>
    <td class="tg-yw4l">yes</td>
  </tr>
  <tr>
    <td class="tg-yw4l">10000</td>
    <td class="tg-yw4l">yes</td>
  </tr>
</table>


<h1>Analysis</h1>
Judging from the data, it looks like our cache can handle a throughput of up to 8695 requests/second reliably. 
