
#include <codepend.h>


cdp::coroutine hello_world()
{
	ttf::instance_counter local_guard;
	std::cout << "hello world" << std::endl;
	co_return;
}

cdp::coroutine satisfy_dependncy(cdp::coroutine_pipe& p, cdp::dependency& d)
{
	ttf::instance_counter local_guard;
	std::cout << "dependency resolved." << std::endl;

	p.resolve(d);
	co_return;
}

cdp::coroutine wait_on_dependency(cdp::dependency& d)
{
	ttf::instance_counter local_guard;
	std::cout << "waiting for dependency." << std::endl;
	co_await d;
	std::cout << "waiting completed..." << std::endl;
	co_return;
}

void basic_coroutine_test()
{
	cdp::coroutine_pipe pipe;

	cdp::dependency dep1;
	pipe.push_back(hello_world());
	pipe.push_back(satisfy_dependncy(pipe, dep1));
	pipe.push_back(wait_on_dependency(dep1));
	pipe.consume_loop([&](cdp::coroutine&& co) {
		pipe.run_frame(std::move(co));
	});
	TEST_ASSERT(pipe.pipe_empty() == true);
	TEST_ASSERT(pipe.dependency_empty() == true);

	{
		pipe.push_back(hello_world());
		TEST_ASSERT(pipe.pipe_empty() == false);
		pipe.consume_loop([&](cdp::coroutine&& co) {
			pipe.run_frame(std::move(co));
		});
	}

	{
		cdp::dependency dep2;
		
		auto cogen = [](cdp::dependency& dep) -> cdp::coroutine {
			ttf::instance_counter local_variable;
			co_await dep;
		};
		{
			auto co = cogen(dep2);
			auto cobk = co;
			pipe.run_frame(std::move(co));
			cobk.handle.promise().waiting_for = nullptr;//to avoid obvois check when coroutines are kiiled when are not done
		}
		
		TEST_ASSERT(pipe.pipe_empty() == true);//because cogen is waiting
		TEST_ASSERT(pipe.dependency_empty() == false);//because this is where cogein is waiting 
		pipe.force_clear_dependency_tasks();
		TEST_ASSERT(pipe.dependency_empty() == true);//because this is where cogein is waiting 
	}

}



void test_main()
{
	TEST_FUNCTION(basic_coroutine_test);
}
TEST_MAIN(test_main)
