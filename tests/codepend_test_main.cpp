
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
		pipe.execute_frame(std::move(co));
	});
	TEST_ASSERT(pipe.pipe_empty() == true);
	TEST_ASSERT(pipe.dependency_empty() == true);

	{
		pipe.push_back(hello_world());
		TEST_ASSERT(pipe.pipe_empty() == false);
		pipe.consume_loop([&](cdp::coroutine&& co) {
			pipe.execute_frame(std::move(co));
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
			pipe.execute_frame(std::move(co));
			cobk.handle.promise().waiting_for = nullptr; //to avoid obvois check when coroutines are killed when are not completed
		}
		
		TEST_ASSERT(pipe.pipe_empty() == true);//because cogen is waiting
		TEST_ASSERT(pipe.dependency_empty() == false);//because this is where cogein is waiting 
		pipe.force_clear_dependency_tasks();
		TEST_ASSERT(pipe.dependency_empty() == true);
	}

}

constexpr std::size_t dcount = 2048;
using dependency_array = std::array<cdp::dependency, dcount>;

cdp::coroutine waiting_coroutine(dependency_array& dvec, std::size_t index, std::atomic<int64_t>& guard, std::atomic<int64_t>& counter)
{
	//wait first
	ttf::instance_counter local_guard;
	co_await dvec[(index * 7901) % dvec.size()];
	int64_t g = guard++;
	if (g > 0)
		counter += g;
	std::cout << ".";
	guard--;
	
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

	std::array<std::thread, 32> threads;

	{
		threading::latch l{ uint32_t(threads.size() + 1) };

		auto thread_job = [&](const std::size_t) {
			pipe.consume_loop_or_wait([&](cdp::coroutine&& co) {
				pipe.execute_frame(std::move(co));
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
		std::atomic<int64_t> counter{ 0 };
		std::atomic<int64_t> guard{ 0 };
		for (std::size_t i = 0; i < darray.size(); i++)
		{
			pipe.push_back(waiting_coroutine(darray, i, guard, counter));
			pipe.push_back(resolving_coroutine(pipe, darray, i));
		}

		pipe.wait_for_empty();
		pipe.evict();
		std::cout << std::endl;
		TEST_ASSERT(counter.load() > 0);
		for (std::size_t i = 0; i < darray.size(); i++)
			TEST_ASSERT(darray[i].resolved() == true);

	}

	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

std::size_t& thread_index()
{
	thread_local std::size_t ti;
	return ti;
}

void test_coroutine_dependency()
{
	cdp::coroutine_pipe pipe;
	std::array<std::thread, 3> threads;

	std::array<cdp::dependency, 3> thread_dependency;

	auto await_for_thread = [&](const std::size_t index)->cdp::coroutine {
		co_await thread_dependency[index];
		std::size_t ti = thread_index();
		TTF_ASSERT(ti == index);
		co_return;
	};

	{
		threading::latch l{ uint32_t(threads.size() + 1) };
		threading::latch e{ uint32_t(threads.size() + 1) };

		auto thread_job = [&](const std::size_t index) {
			pipe.resolve_in_frame(thread_dependency[index]);
		};

		for (std::size_t i = 0; i < threads.size(); i++)
		{
			auto t = std::thread([&, index = i]() {
				thread_index() = index;
				l.arrive_and_wait();
				thread_job(index);
				e.arrive_and_wait();
				});
			threads[i].swap(t);
		}

		for (std::size_t i = 0; i < threads.size(); i++)
		{
			pipe.push_back(thread_dependency[i], await_for_thread(i));
		}

		l.arrive_and_wait();

		e.arrive_and_wait();

		for (std::size_t i = 0; i < thread_dependency.size(); i++)
			TEST_ASSERT(thread_dependency[i].resolved() == true);
	}

	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();


}

void test_main()
{
	TEST_FUNCTION(test_coroutine_dependency);
	TEST_FUNCTION(basic_coroutine_test);
	TEST_FUNCTION(test_more_coroutines);
}
TEST_MAIN(test_main)
