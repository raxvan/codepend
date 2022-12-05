
#include <codepend.h>
#include <array>
#include <iostream>
#include <chrono>

#include <threading.h>

struct copipe : public cdp::coroutine_pipe, public threading::async_pipe<cdp::coroutine>
{
	virtual void push_async(cdp::coroutine&& co) override
	{
		threading::async_pipe<cdp::coroutine>::push_back(co);
	}
};

void thread_sleep(int ms_time)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms_time));
}

//--------------------------------------------------------------------------------------------------------------------------------

cdp::coroutine hello_world()
{
	ttf::instance_counter local_guard;
	std::cout << "hello world" << std::endl;
	co_return;
}

cdp::coroutine satisfy_dependncy(cdp::dependency& d)
{
	ttf::instance_counter local_guard;
	std::cout << "resolving dependency..." << std::endl;
	co_yield d;
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
	copipe pipe;

	{
		cdp::dependency dep1;
		pipe.push_async(hello_world());
		pipe.push_async(satisfy_dependncy(dep1));
		pipe.push_async(wait_on_dependency(dep1));
		pipe.consume_loop([&](cdp::coroutine&& co) { pipe.execute_frame(co); });
		TEST_ASSERT(pipe.empty() == true);

		{
			pipe.push_async(hello_world());
			TEST_ASSERT(pipe.empty() == false);
			pipe.consume_loop([&](cdp::coroutine&& co) { pipe.execute_frame(co); });
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
				pipe.execute_frame(co);
			}

			pipe.resolve_in_frame(dep2);

			TEST_ASSERT(pipe.empty() == true); // because cogen is waiting
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------------------

constexpr std::size_t dcount = 2048;
using dependency_array = std::array<cdp::dependency, dcount>;

cdp::coroutine waiting_coroutine(dependency_array& dvec, std::size_t index, std::atomic<int64_t>& guard, std::atomic<int64_t>& counter)
{
	// wait first
	ttf::instance_counter local_guard;
	co_await dvec[(index * 7901) % dvec.size()];
	int64_t g = guard++;
	if (g > 0)
		counter += g;
	std::cout << ".";
	guard--;
}
cdp::coroutine resolving_coroutine(dependency_array& dvec, std::size_t index)
{
	// wait first
	ttf::instance_counter local_guard;
	co_yield dvec[(index * 6949) % dvec.size()];
	co_return;
}

void test_more_coroutines()
{
	copipe pipe;

	std::array<std::thread, 32> threads;

	{
		threading::latch l { uint32_t(threads.size() + 1) };

		auto thread_job = [&](const std::size_t) { pipe.consume_loop_or_wait([&](cdp::coroutine&& co) { pipe.execute_frame(co); }); };

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

		std::atomic<int64_t> counter { 0 };
		std::atomic<int64_t> guard { 0 };
		for (std::size_t i = 0; i < darray.size(); i++)
		{
			pipe.push_async(waiting_coroutine(darray, i, guard, counter));
			pipe.push_async(resolving_coroutine(darray, i));
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

//--------------------------------------------------------------------------------------------------------------------------------

std::size_t& thread_index()
{
	thread_local std::size_t ti;
	return ti;
}

void test_coroutine_dependency()
{
	copipe					   pipe;
	std::array<std::thread, 3> threads;

	std::array<cdp::dependency, 3> thread_dependency;

	auto await_for_thread = [&](const std::size_t index) -> cdp::coroutine {
		co_await thread_dependency[index];
		std::size_t ti = thread_index();
		TTF_ASSERT(ti == index);
		co_return;
	};

	{
		threading::latch l { uint32_t(threads.size() + 1) };
		threading::latch e { uint32_t(threads.size() + 1) };

		auto thread_job = [&](const std::size_t index) { pipe.resolve_in_frame(thread_dependency[index]); };

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
			thread_dependency[i].add(await_for_thread(i));
		}

		l.arrive_and_wait();

		e.arrive_and_wait();

		for (std::size_t i = 0; i < thread_dependency.size(); i++)
			TEST_ASSERT(thread_dependency[i].resolved() == true);
	}

	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

//--------------------------------------------------------------------------------------------------------------------------------

cdp::coroutine wait(const std::size_t index)
{
	thread_sleep(12 + int(index % 13));
	co_return;
}

void test_cosginal()
{
	copipe pipe;

	std::array<std::thread, 32> threads;

	{
		threading::latch l { uint32_t(threads.size() + 1) };

		auto thread_job = [&](const std::size_t) { pipe.consume_loop_or_wait([&](cdp::coroutine&& co) { pipe.execute_frame(co); }); };

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

	cdp::cosignal wait_first;
	cdp::cosignal wait_second;

	for (std::size_t i = 0; i < 100; i++)
	{
		if (i % 2 == 0)
			pipe.push_async(wait(i) + wait_first);
		else
			pipe.push_async(wait(i) + wait_second);
	}

	TEST_ASSERT(wait_first.wait() == true);
	pipe.wait_for_empty();
	TEST_ASSERT(wait_second.wait() == false);
	pipe.evict();
	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

//--------------------------------------------------------------------------------------------------------------------------------

cdp::coroutine setter(cdp::result<uint32_t>& one, cdp::result<uint32_t>& two, cdp::result<uint32_t>& three)
{
	co_yield one = 1;
	co_yield two = 2;
	co_yield three = 3;
}

cdp::coroutine getter(cdp::result<uint32_t>& d, const uint32_t expected)
{
	uint32_t v = co_await d;
	TEST_ASSERT(v == expected);
}
void test_dependency_value()
{
	copipe pipe;

	cdp::result<uint32_t> one;
	cdp::result<uint32_t> two;
	cdp::result<uint32_t> three;

	pipe.push_async(setter(one, two, three));
	pipe.push_async(getter(one, 1));
	pipe.push_async(getter(two, 2));
	pipe.push_async(getter(three, 3));

	pipe.consume_loop([&](cdp::coroutine&& co) { pipe.execute_frame(co); });
}

//--------------------------------------------------------------------------------------------------------------------------------

cdp::coroutine string_setter(cdp::result<std::string>& h, cdp::result<std::string>& w)
{
	std::cout << co_yield h = "hello ";
	co_yield w = "world\n";
}
cdp::coroutine writer(cdp::result<std::string>& d)
{
	std::cout << co_await d;
}

void test_string_value()
{
	copipe pipe;

	cdp::result<std::string> h;
	cdp::result<std::string> w;

	pipe.push_async(string_setter(h, w));
	pipe.push_async(writer(h));

	pipe.consume_loop([&](cdp::coroutine&& co) { pipe.execute_frame(co); });
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_frames()
{
	copipe pipe;

	std::array<std::thread, 32> threads;

	{
		threading::latch l { uint32_t(threads.size() + 1) };

		auto thread_job = [&](const std::size_t index) {
			thread_index() = index + 1;
			pipe.consume_loop_or_wait([&](cdp::coroutine&& co) { pipe.execute_frame(co); });
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

	cdp::frame main_frame;
	cdp::frame exit_frame;

	auto cojob = [](auto& mf, auto& ef, bool& l) -> cdp::coroutine {
		while (true)
		{
			ttf::instance_counter _refcheck;
			std::size_t			  ti = thread_index();
			TEST_ASSERT(thread_index() != 0);
			co_await mf;
			TEST_ASSERT(thread_index() == 0);
			if (l == false)
				break;
			co_await ef;
			thread_sleep(int(5 + ti % 4));
		}
		co_return;
	};

	bool keep_looping = true;
	for (std::size_t i = 0; i < 128; i++)
	{
		pipe.push_async(cojob(main_frame, exit_frame, keep_looping));
	}

	thread_index() = 0;
	for (std::size_t i = 0; i < 10; i++)
	{
		thread_sleep(3);
		pipe.execute_in_frame(main_frame);
		pipe.execute_in_queue(exit_frame);
	}
	pipe.wait_for_empty();

	pipe.execute_in_frame(exit_frame);
	keep_looping = false;
	pipe.execute_in_frame(main_frame);

	pipe.evict();
	for (std::size_t i = 0; i < threads.size(); i++)
		threads[i].join();
}

//--------------------------------------------------------------------------------------------------------------------------------

cdp::coroutine accumulator(std::size_t& out)
{
	out++;
	co_return;
};

void test_coroutine_generator()
{
	copipe pipe;

	std::size_t counter = 0;

	auto main_coroutine = [](std::size_t& out) -> cdp::coroutine {
		for (std::size_t i = 0; i < 10; i++)
		{
			auto g = [&](cdp::coroutine& co, cdp::coroutine_pipe&) -> bool {
				auto acc = accumulator(out);
				co.handle.promise().add_sequential(acc.detach());
				return true;
			};
			co_yield cdp::coroutine::frame_function(std::move(g));
		}

		co_return;
	};

	pipe.push_async(main_coroutine(counter));
	pipe.consume_loop([&](cdp::coroutine&& co) { pipe.execute_frame(co); });

	TEST_ASSERT(counter == 10);
}

void test_main()
{
	TEST_FUNCTION(test_coroutine_dependency);
	TEST_FUNCTION(basic_coroutine_test);
	TEST_FUNCTION(test_more_coroutines);
	TEST_FUNCTION(test_cosginal);
	TEST_FUNCTION(test_dependency_value);
	TEST_FUNCTION(test_string_value);
	TEST_FUNCTION(test_frames);
	TEST_FUNCTION(test_coroutine_generator);
}
TEST_MAIN(test_main)
