
#include <codepend.h>



cdp::coroutine hello_world()
{
	std::cout << "hello world" << std::endl;
	co_return;
}

cdp::coroutine satisfy_dependncy(cdp::dependency_machiene& dm, cdp::dependency* dep)
{
	std::cout << "dependency " << dep->id << 
	dm.satisfy(dep);
	co_return;
}



void test_main()
{
	cdp::dependency_machine mac;
	
	std::vector<cdp::coroutine> free_tasks;

	auto push_task = [&](cdp::coroutine&& co){
		free_tasks.push_back(std::move(co));
	};

	
	push_task(hello_world());

	while(free_tasks.size())
	{
		auto co = std::move(free_tasks.back());
		free_tasks.pop_back();

		co.handle();
		
		if(co.handle.done())
			continue;

		auto* dependency = co.handle.promise().waiting_for;
		TEST_ASSERT(dependency != nullptr);

		mac.push_task(*dependency, std::move(co));
	}

}
TEST_MAIN(test_main)
