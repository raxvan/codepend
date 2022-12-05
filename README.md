
# codepend

So about this repository ... it's about c++20 coroutines and threads. If you want a threaded job system done right, there is no better alternative to coroutines(or fibers) running on fixed number of threads. For the implementation of this abomination, i'm using (the not so new) c++20 coroutines, a dependency system, and a multiple producer/consumer pipe for execution.

Here is an example:
```
cdp::coroutine satisfy_dependncy(cdp::dependency& d)
{
	std::cout << "resolving dependency..." << std::endl;
	co_yield d; //resolve dependency
	std::cout << "dependency resolved." << std::endl;
	co_return;
}

cdp::coroutine wait_on_dependency(cdp::dependency& d)
{
	std::cout << "waiting for dependency." << std::endl;
	co_await d;
	std::cout << "waiting completed..." << std::endl;
	co_return;
}

void execute_all()
{
	cdp::coroutine_pipe pipe;

	cdp::dependency dep1;
	pipe.push_async(satisfy_dependncy(pipe, dep1));
	pipe.push_async(wait_on_dependency(dep1));
	pipe.consume_loop([&](cdp::coroutine&& co) {
		pipe.execute_frame(co);
	});
}

```
output:
```
waiting for dependency.
resolving dependency...
dependency resolved.
waiting completed...
```

but you can use multiple threads for this shenanigan:
```
//multiple threads doing this:
pipe.consume_loop_or_wait([&](cdp::coroutine&& co) {
	pipe.execute_frame(co);
});
```

# on the TODO list:

1. Address the coroutine allocator issue:
	
	Currently the default new/delete operators are used and this needs to addressed. C++20 coroutines offer poor support for custom allocators for coroutines, and this makes me sad, but i have an idea ... using thread local storage ...

3. More safety and tooling:

	Tooling to identify logic deadlocks, dependency tracking, and more testing.

4. More compiler support:
	
	Currently this was developed/tested on msvc 2022. I want to see this running on more compilers.

