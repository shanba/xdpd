#include "bufferpool.h"
#include "dpdk_datapacket.h"

using namespace xdpd::gnu_linux;
using namespace xdpd::gnu_linux_dpdk;

/* Static member initialization */
bufferpool* bufferpool::instance = NULL;
pthread_mutex_t bufferpool::mutex = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t bufferpool::cond = PTHREAD_COND_INITIALIZER;
 
//Constructor and destructor
bufferpool::bufferpool(long long unsigned int pool_items)
	: pool(pool_items),
	pool_status(pool_items)
{

	long long unsigned int i;
	datapacket_t* dp;
	datapacketx86* dpx86;
	dpdk_pkt_platform_state_t* state;	

	for(i=0;i<pool_items;++i){

		//Init datapacket
		dp = (datapacket_t*)malloc(sizeof(datapacket_t));
	
		if(!dp){
			//Mark as unavailable
			pool[i] = NULL;
			pool_status[i] = BUFFERPOOL_SLOT_UNAVAILABLE;

			//Skip
			continue;
		}
			
		state = (dpdk_pkt_platform_state_t*)malloc(sizeof(dpdk_pkt_platform_state_t));
		
		if(!state){
			//Mark as unavailable
			pool[i] = NULL;
			pool_status[i] = BUFFERPOOL_SLOT_UNAVAILABLE;
			
			free(dp);
			//Skip
			continue;
		}

		//Memset datapacket
		memset(dp,0,sizeof(*dp));
		
		//Init datapacketx86
		try {
			dpx86 = new datapacketx86();
		}catch(std::bad_alloc ex){

			//Mark as unavailable
			pool[i] = NULL;
			pool_status[i] = BUFFERPOOL_SLOT_UNAVAILABLE;
			
			free(dp);
			continue;		
		}		

		//Assign the buffer_id
		dpx86->internal_buffer_id = i;			
		dpx86->buffer_id = 0; //Mark as 0
		
		//Link them
		state->pktx86 = dpx86;	
		dp->platform_state = (platform_datapacket_state_t*)state;

		//Add to the pool	
		pool[i] = dp;
		pool_status[i] = BUFFERPOOL_SLOT_AVAILABLE;
	
	}

	//Set size
	pool_size = pool_items;
	next_index = 0;
#ifdef DEBUG
	used = 0;
#endif
}

bufferpool::~bufferpool(){
	
	unsigned long long int i;
	dpdk_pkt_platform_state_t* state;	

	for(i=0;i<pool.size();++i){
		if(pool[i]){
			state = (dpdk_pkt_platform_state_t*)pool[i]->platform_state;
			delete state->pktx86;
			free(state);
			free(pool[i]);
		}	
	}
}



//
// Buffer pool management
//
void bufferpool::init(long long unsigned int capacity){
	

	pthread_mutex_lock(&bufferpool::mutex);		

	//Add the RESERVED_SLOTS to the capacity 
	capacity += RESERVED_SLOTS;

	if(bufferpool::instance){
		//Double-call to init??
		ROFL_DEBUG("Double call to bufferpool init!! Skipping...\n");
		pthread_mutex_unlock(&bufferpool::mutex);
		return;	
	}
	
	ROFL_DEBUG("Initializing bufferpool with a capacity of %d buffers\n",capacity);

	//Init 	
	bufferpool::instance = new bufferpool(capacity);

	//Wake consumers
	pthread_cond_broadcast(&bufferpool::cond);

	//Release and go!	
	pthread_mutex_unlock(&bufferpool::mutex);		
}

void bufferpool::destroy(){

	if(get_instance())
		delete get_instance();

	instance = NULL;	
}

/*
* Resizes current buffer pool to new_capacity, to the final size of new_capacity+RESERVED_SLOTS
*/
void bufferpool::increase_capacity(long long unsigned int new_capacity){
	assert(0);
}
