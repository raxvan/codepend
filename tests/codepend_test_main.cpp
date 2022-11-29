
#include <codepend.h>

struct task_pipe
{
public:
	std::vector<cdp::coroutine> free_tasks;
public:
	void push_back(cdp::coroutine&& co)
	{
		free_tasks.push_back(std::move(co));
	};

	bool empty() const
	{
		return free_tasks.size() == 0;
	}

	cdp::coroutine pop_back()
	{
		auto co = std::move(free_tasks.back());
		free_tasks.pop_back();
		return co;
	}
};

cdp::coroutine hello_world()
{
	std::cout << "hello world" << std::endl;
	co_return;
}
/*
cdp::coroutine satisfy_dependncy(cdp::dependency_machine& dm, task_pipe& pipe, cdp::dependency* dep)
{
	std::cout << "dependency resolved: " << dep->id << std::endl;

	dm.resolve(pipe, *dep, uint64_t(1));
	co_return;
}

cdp::coroutine wait_on_dependency(cdp::dependency* dep)
{
	std::cout << "waiting for: " << dep->id << std::endl;

	co_await *dep;

	std::cout << "waiting completed: " << dep->id << std::endl;
	
	co_return;
}
*/


void test_main()
{
	
	task_pipe pipe;
	
	cdp::dependency prop;
	
	//pipe.push_back(satisfy_dependncy(dm, pipe, &prop));
	//pipe.push_back(wait_on_dependency(&prop));

	while(pipe.empty() == false)
	{
		auto co = pipe.pop_back();

		co.handle();
		
		if(co.handle.done())
			continue;

		auto* dependency = co.handle.promise().waiting_for;
		TEST_ASSERT(dependency != nullptr);
		//dm.push_task(*dependency, std::move(co));
	}

}
TEST_MAIN(test_main)
