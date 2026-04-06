cmake_minimum_required(VERSION 3.16)
project(matcher)

set(CMAKE_CXX_STANDARD 20)

include_directories(
	.
	core
	engine
	queue
	memory
)

add_executable(matcher
   	main.cpp
   	engine/MarketMatcher.cpp
   	queue/LockFreeQueue.cpp
   	memory/OrderPool.cpp
   	memory/MMapWriter.cpp
	
)
add_executable(benchmark
   	bench/benchmark.cpp
   	engine/MarketMatcher.cpp
	queue/LockFreeQueue.cpp
    	memory/OrderPool.cpp
    	memory/MMapWriter.cpp
)