
#include <codepend.h>
#include <array>
#include <iostream>

cdp::coroutine hello_world()
{
	ttf::instance_counter local_guard;
	std::cout << "hello world" << std::endl;
	co_return;
}

cdp::coroutine satisfy_dependncy(cdp::coroutine_pipe& p, cdp::dependency& d)
{
	ttf::instance_counter local_guard;
	std::cout << "resolving dependency..." << std::endl;
	p.resolve(d);
	std::cout << "dependency resolved." << std::endl;
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
			cobk.handle.promise().waiting_for = nullptr; //to avoid obvois check when coroutines are killed when are not completed
		}
		
		TEST_ASSERT(pipe.pipe_empty() == true);//because cogen is waiting
		TEST_ASSERT(pipe.dependency_empty() == false);//because this is where cogein is waiting 
		pipe.force_clear_dependency_tasks();
		TEST_ASSERT(pipe.dependency_empty() == true);
	}

}

constexpr std::size_t dcount = 128;
using dependency_array = std::array<cdp::dependency, dcount>;

cdp::coroutine waiting_coroutine(dependency_array& dvec, std::size_t index)
{
	//wait first
	ttf::instance_counter local_guard;
	co_await dvec[(index * 7901) % dvec.size()];
	std::cout << index << std::endl;
	
}
cdp::coroutine resolving_coroutine(cdp::coroutine_pipe& p, dependency_array& dvec, std::size_t index)
{
	//wait first
	ttf::instance_counter local_guard;
	p.resolve(dvec[(index * 6949) % dvec.size()]);
	co_return;
}

void test_more_coroutines()
{
	cdp::coroutine_pipe pipe;

	std::array<std::thread, 3> threads;

	{
		threading::latch l{ uint32_t(threads.size() + 1) };

		auto thread_job = [&](const std::size_t) {
			pipe.consume_loop_or_wait([&](cdp::coroutine&& co) {
				pipe.run_frame(std::move(co));
			});
		};

		for (std::size_t i = 0; i < threads.size(); i++)
		{
			auto t = std::thread([&, index = i]() {
				l.arrive_and_wait();
				thread_job(index);
				});
			threads[i].swap(t);
		}
		l.arrive_and_wait();
	}

	{
		dependency_array darray;

		for (std::size_t i = 0; i < darray.size(); i++)
		{
			pipe.push_back(waiting_coroutine(darray, i));
			pipe.push_back(resolving_coroutine(pipe, darray, i));
		}

		pipe.wait_for_empty();
		pipe.evict();

		for (std::size_t i = 0; i < darray.size(); i++)
			TEST_ASSERT(darray[i].resolved() == true);

	}

	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

void test_main()
{
	TEST_FUNCTION(basic_coroutine_test);
	TEST_FUNCTION(test_more_coroutines);
}
TEST_MAIN(test_main)
