
#include <codepend.h>


cdp::coroutine hello_world()
{
	std::cout << "hello world" << std::endl;
	co_return;
}

cdp::coroutine satisfy_dependncy(cdp::coroutine_pipe& p, cdp::dependency& d)
{
	std::cout << "dependency resolved: " << d.id << std::endl;

	p.resolve(d);
	co_return;
}

cdp::coroutine wait_on_dependency(cdp::dependency& d)
{
	std::cout << "waiting for: " << d.id << std::endl;

	co_await d;

	std::cout << "waiting completed: " << d.id << std::endl;
	
	co_return;
}



void test_main()
{
	cdp::coroutine_pipe pipe;
	
	cdp::dependency prop;
	
	pipe.push_back(hello_world());
	pipe.push_back(satisfy_dependncy(pipe, prop));
	pipe.push_back(wait_on_dependency(prop));

	pipe.consume_loop([&](cdp::coroutine&& co) {
		pipe.run_frame(std::move(co));
	});

	

}
TEST_MAIN(test_main)
