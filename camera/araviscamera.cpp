#include "araviscamera.h"
#include <cstring>

#define TIMEOUT 3
#define BUFFERQ 5

AravisCam::AravisCam(){}

AravisCam:: ~AravisCam(){
	endCam();
	delete rawbuffer;
}

bool AravisCam::initializeCam(){
	ArvGcNode *feature;
    ArvCamera *payload_camera;
	std::vector<std::string> settings; 
	GType value_type;
	bool config_ok;
	int width, height, packet_size;

//  Initial Setup - Find the Camera	
	std::cout<<"Looking for the Camera...\n";
	payload_camera = arv_camera_new(NULL);
	device = arv_camera_get_device(payload_camera); //arv_open_device(NULL);
	
	if(device == NULL) {
		std::cout<< "No camera found!" << std::endl;
		return false;	
	}
	
	acquisition = false;

	std::cout<< "Found "<< arv_get_device_id(0) << std::endl;
	genicam = arv_device_get_genicam(device);

    std::cout << "ArvCameraPxFormat: " << arv_camera_get_pixel_format_as_string(payload_camera) << std::endl;

    feature = arv_gc_get_node(genicam, "GevStreamChannelCount");
    packet_size = arv_gc_integer_get_value(ARV_GC_INTEGER (feature), NULL);
    std::cout << "GevStreamChannelCount: " << packet_size <<  std::endl; 

    feature = arv_gc_get_node(genicam, "dataStreamSelector");
    packet_size = arv_gc_integer_get_value(ARV_GC_INTEGER (feature), NULL);
    std::cout << "dataStreamSelector: " << packet_size <<  std::endl; 

    feature = arv_gc_get_node(genicam, "dataStreamType");
    packet_size = arv_gc_integer_get_value(ARV_GC_INTEGER (feature), NULL);
    std::cout << "dataStreamType: " << packet_size <<  std::endl; 

	feature = arv_gc_get_node(genicam,"Width");
	width = arv_gc_integer_get_value(ARV_GC_INTEGER (feature), NULL);
		
	feature = arv_gc_get_node(genicam,"Height");
	height = arv_gc_integer_get_value(ARV_GC_INTEGER (feature), NULL);

	feature = arv_gc_get_node (genicam, "PayloadSize");
    arv_gc_integer_set_value(ARV_GC_INTEGER(feature), 2354176, NULL);
	payload = arv_gc_integer_get_value (ARV_GC_INTEGER (feature), NULL);

	std::cout<<"Image " << width << "x" << height << ", " << payload << " bytes " << std::endl;

	//Create Stream and fill buffer queue
	stream = arv_device_create_stream (device, NULL, NULL);

        for (int i = 0; i < BUFFERQ; i++)
		arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));
	
	//Allocate buffer and size
	rawbuffer = new unsigned char[1936*1216];	
	size = cv::Size(width, height);

	//Get and save the node that is the software trigger
	triggernode = arv_gc_get_node(genicam,"TriggerSoftware");

	return true;
}

void AravisCam::trigger(){
	if(acquisition == false)
		startCam();

	arv_gc_command_execute(ARV_GC_COMMAND(triggernode),NULL);
	std::cout<< "Sent software trigger" << std::endl;
}

bool AravisCam::getBuffer(){
	ArvBuffer * arvbufr;
	bool gotbuf = false;
	int cycles = 0;

	std::cout<<"Getting Buffer...";
	do {
        g_usleep(50000);
		cycles++;
		do  {
		for (int i = 0; i < BUFFERQ; i++){
			arvbufr = arv_stream_try_pop_buffer (stream);
			if (arvbufr != NULL){
				switch(arvbufr->status){
					case ARV_BUFFER_STATUS_SUCCESS: std::cout<<"Success"<<std::endl; break;
					case ARV_BUFFER_STATUS_TIMEOUT: std::cout<<"Timeout"<<std::endl; break;
					default: std::cout<<"Error"<<std::endl;;
				}
				if (arvbufr->status == ARV_BUFFER_STATUS_SUCCESS){
					memcpy(rawbuffer,arvbufr->data,payload);
					gotbuf = true;			
				}	 
				arv_stream_push_buffer (stream, arvbufr);
			}		 
		}
		} while (arvbufr != NULL && !gotbuf);
	}while(cycles < TIMEOUT && !gotbuf);

	return gotbuf;
}

void  AravisCam::startCam(){
	ArvGcNode *start = arv_gc_get_node(genicam, "AcquisitionStart");
	arv_gc_command_execute( ARV_GC_COMMAND(start),NULL);
	std::cout<< "Beginning camera acquisition"<<std::endl;
	acquisition = true;
	g_usleep(50000);
}

void AravisCam::endCam(){
	ArvGcNode *end;
	guint64 n_processed_buffers, n_failures, n_underruns;

	arv_stream_get_statistics (stream, &n_processed_buffers, &n_failures, &n_underruns);
	std::cout << "Processed\t = " << (unsigned int) n_processed_buffers << "\n";
	std::cout << "Failures\t  = " << (unsigned int) n_failures << "\n";
	std::cout << "Underruns\t = " << (unsigned int) n_underruns << "\n";

	end = arv_gc_get_node (genicam, "AcquisitionStop");
	arv_gc_command_execute (ARV_GC_COMMAND (end), NULL);
	std::cout << "Ended Camera Acquisition" << std::endl;
	acquisition = false;

	g_object_unref (stream);
	g_object_unref (device);
}

bool AravisCam::getImage(cv::Mat &frame){	

	bool image_ok = getBuffer();
	
	if(image_ok){
		rawmat = cv::Mat(size,CV_8UC1,rawbuffer);
		cv::cvtColor(rawmat,frame,CV_BayerGB2RGB);	//Average Time = ~15 ms 
		
		if (frame.empty())
		{
			image_ok = false;
		}
	}
	
	return image_ok;
}

