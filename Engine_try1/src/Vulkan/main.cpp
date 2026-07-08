#include "vk_application.h"

int main(int argc, char* argv[]){
	VK_INIT_ENGINE::VulkanInitEngine init_engine(true);
	if (!init_engine.Get()._isInitialized) {
		fmt::print("ERROR: Engine don't initialized\n");
		return -1;
	}

	VK_APPLICATION::VulkanApplication MyApplication(init_engine.Get());
	MyApplication.run();
	MyApplication.cleanup();
	init_engine.init_cleanup();
	return 0;
}