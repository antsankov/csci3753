/* While The Queue Is Not Empty */
	while(!queue_is_empty(parameters->q) || !finished){

		/* Lock The Queue So Only This Thread Can Access It */
    	mutex_lock_error = pthread_mutex_lock(queue_mutex);
    	if (mutex_lock_error){
			fprintf(stderr, "ERROR; return code from pthread_mutex_lock() for the queue is %d\n", mutex_lock_error);
		} 

    	/* Get Hostname Off Queue */
    	payload = queue_pop(parameters->q);

    	if(payload == NULL){
    		mutex_unlock_error = pthread_mutex_unlock(queue_mutex);
			if (mutex_unlock_error){
	    		fprintf(stderr, "ERROR; return code from pthread_mutex_unlock() for the queue is %d\n", mutex_unlock_error);
			} 
    		fprintf(stderr, "Unable to pop anything off the queue becuase the queue is empty\n");
    		usleep(100);
    	}
    	else {
    	/* Unlock The Queue */
		mutex_unlock_error = pthread_mutex_unlock(queue_mutex);
		if (mutex_unlock_error){
	    	fprintf(stderr, "ERROR; return code from pthread_mutex_unlock() for the queue is %d\n", mutex_unlock_error);
		} 

		/* Lookup hostname and get IP string */
	    if(dnslookup(payload, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
			fprintf(stderr, "dnslookup error: %s\n", payload);
			strncpy(firstipstr, "", sizeof(firstipstr));
	    }

	    /* Lock Output File In Order To Write To It */
	    mutex_lock_error = pthread_mutex_lock(file_mutex);
	    if (mutex_lock_error){
	    	fprintf(stderr, "ERROR; return code from pthread_mutex_lock() for the output file is %d\n", mutex_lock_error);
		} 

	    /* Write to Output File */
	    fprintf(resolver_thread_file, "%s,%s\n", payload, firstipstr);

	    /* Unlock Output File So Other Threads Can Write To It */
	    mutex_unlock_error = pthread_mutex_unlock(file_mutex);
	    if (mutex_unlock_error){
	    	fprintf(stderr, "ERROR; return code from pthread_mutex_unlock() for the output file is %d\n", mutex_unlock_error);
		} 

	    /* Free Memory Blocks On The Heap Created By Payload */
	    free(payload);
		}
	}