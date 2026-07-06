#include "vk_init_engine.h"

int main(int argc, char* argv[]){
	VK_INIT_ENGINE::VulkanInitEngine init_engine(true);
	init_engine.Get();
	return 0;
}