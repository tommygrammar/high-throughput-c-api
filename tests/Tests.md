## The javascript files are to test whether the apis are working once you have them running.

## I have also included a basic Apache Jmeter test for load testing under different conditions. These are the results I was able to achieve:

### Conditons: 2 Seconds Ramp Up with 10 loops

~ 50 concurrent users -> 254.4 throughputs/sec
~ 100 concurrent users -> 503.8 throughputs/sec
~ 200 concurrent users -> 1002 throughputs/sec
~ 300 concurrent users -> 1429.3 throughputs/sec
~ 400 concurrent users -> 1042 throughputs/sec
~ 500 concurrent users -> 1252 throughputs/sec
~ 800 concurrent users -> 1464 throughputs/sec

### All with an error rate of 0.00%
