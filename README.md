Trading platform simulator

v2 was simplified to run fully on io_context without use of custom sinks and lock free queues.<br> 
Tickers are divided between threads, one context per worker thread and one network context.<br>

Last performance check for 5us trade rate and 4 server cores:<br>
Server:<br>
22:25:08.702500 [I] Orders [matched|total] 2307286 2961784 rps:101929<br>
Trader:<br>
22:25:08.677493 [I] RTT [1us|100us|1ms]  100.00% avg:18us  0.00% avg:135us  0.00% avg:0ms<br>
