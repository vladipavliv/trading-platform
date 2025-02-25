Trading platform simulator

v2 was simplified to run fully on io_context without use of custom sinks and lock free queues.<br> 
Tickers are divided between threads, one context per worker thread and one network context.<br>

Last performance check:<br>
Server:<br>
21:18:44.947956 [I] Orders [matched|total] 3872996 4954931 rps:82520<br>
Trader:<br>
21:18:44.391349 [I] RTT [1us|100us|1ms]  99.84% avg:24us  0.16% avg:141us  0.00% avg:0ms<br>
