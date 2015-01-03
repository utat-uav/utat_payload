#include "arvteledyne.h"

Teledyne::Teledyne(){
	buffer = NULL;
}

Teledyne:: ~Teledyne(){
	if(buffer) delete buffer;
}

bool Teledyne::initCamSetting(){
	ArvGcNode *feature;
	std::vector<std::string> settings, values;
	GType value_type;

//  Initial Setup - Find the Camera	
	std::cout<<"Looking for the Camera ...\n";
	device = arv_open_device(NULL);

	if(device == NULL) {
		std::cout<< "No camera found!" << std::endl;
		return false;	
	}
	std::cout<< "Found "<< arv_get_device_id(0) << std::endl;
	genicam = arv_device_get_genicam(device);

// Parse inputs from a configuration file
  	parseInputs(settings,values);
	if (settings.size() == 0 || (settings.size() != values.size())){
		std::cout <<"Error parsing configuration file" << std::endl;
		return false;
	}

//Apply setting and display to confirm
	for  (std::vector<std::string>::size_type i = 0; i != settings.size(); i++){
		feature = arv_gc_get_node(genicam,settings[i].c_str());

		if (ARV_IS_GC_FEATURE_NODE (feature)) {
			if (ARV_IS_GC_COMMAND (feature)) std::cout<< settings[i] << " is a command" <<std::endl;
			else {
				arv_gc_feature_node_set_value_from_string (ARV_GC_FEATURE_NODE (feature), values[i].c_str(), NULL);
				
				value_type = arv_gc_feature_node_get_value_type (ARV_GC_FEATURE_NODE (feature));
				std::cout << settings[i] << " = ";
				switch (value_type) { 
				 case G_TYPE_INT64:
					std::cout << arv_gc_integer_get_value(ARV_GC_INTEGER(feature),NULL) << std::endl;
			 	 	break;
			 	 case G_TYPE_DOUBLE:
					std::cout << arv_gc_float_get_value(ARV_GC_FLOAT(feature),NULL) << std::endl;
			 		break;
				 case G_TYPE_STRING:
					std::cout << arv_gc_string_get_value(ARV_GC_STRING(feature),NULL) << std::endl;
					break;
				 case G_TYPE_BOOLEAN:
					std::cout << arv_gc_integer_get_value(ARV_GC_INTEGER(feature),NULL) << std::endl;
					break;
				 default:
					std::cout << arv_gc_feature_node_get_value_as_string(ARV_GC_FEATURE_NODE(feature),NULL) << std::endl; 
				}
			}
		}
	}

	feature = arv_gc_get_node (genicam, "PayloadSize");
	payload = arv_gc_integer_get_value (ARV_GC_INTEGER (feature), NULL);
	std::cout<< "PayloadSize = " << payload << std::endl;

	buffer = new unsigned char[payload];

	//Create Stream and fill buffer queue
	stream = arv_device_create_stream (device, NULL, NULL);

        for (int i = 0; i < BUFFER_Q_SIZE; i++)
		arv_stream_push_buffer (stream, arv_buffer_new (payload, NULL));
	
	//Get and save the node that is the software trigger
	trigger = arv_gc_get_node(genicam,"TriggerSoftware");

	return true;
}

void Teledyne::startCam(){
	ArvGcNode *start = arv_gc_get_node(genicam, "AcquisitionStart");
	arv_gc_command_execute( ARV_GC_COMMAND(start),NULL);
	std::cout<< "Beginning camera acquisition"<<std::endl;
}

void Teledyne::sendTrigger(){
	arv_gc_command_execute(ARV_GC_COMMAND(trigger),NULL);
	std::cout<< "Sent software trigger" << std::endl;
}


unsigned char* Teledyne::getBuffer(){
	ArvBuffer * arvbufr;
	bool snapped = false;
	int cycles = 0;
	do {
		g_usleep (10000);
		cycles++;
		do  {
			arvbufr = arv_stream_try_pop_buffer (stream);
			if (arvbufr != NULL){
				std::cout<<"Buffer: ";
				switch(arvbufr->status){
					case ARV_BUFFER_STATUS_SUCCESS: std::cout<<"buffer success"<<std::endl; break;
					case ARV_BUFFER_STATUS_TIMEOUT: std::cout<<"timeout"<<std::endl; break;
					default: std::cout<<"error"<<std::endl;;
				}
				if (arvbufr->status == ARV_BUFFER_STATUS_SUCCESS){
					memcpy(buffer,arvbufr->data,payload);
					snapped = true;			
				}	 
				arv_stream_push_buffer (stream, arvbufr);
			}		 
		} while (arvbufr != NULL);
	}while(cycles < WAIT_CYCLES && !snapped);

	return buffer;
}

void Teledyne::endCam(){
	ArvGcNode *end;
	guint64 n_processed_buffers, n_failures, n_underruns;

	arv_stream_get_statistics (stream, &n_processed_buffers, &n_failures, &n_underruns);
	std::cout << "Processed buffers = " << (unsigned int) n_processed_buffers << "\n";
	std::cout << "Failures 	   	= " << (unsigned int) n_failures << "\n";
	std::cout << "Underruns 	= " << (unsigned int) n_underruns << "\n";

	end = arv_gc_get_node (genicam, "AcquisitionStop");
	arv_gc_command_execute (ARV_GC_COMMAND (end), NULL);
	std::cout << "Ended Camera Acquisition" << std::endl;

	g_object_unref (stream);
	g_object_unref (device);
}

void Teledyne::parseInputs(std::vector<std::string> &settings, std::vector<std::string> &values){
	std::ifstream cfgstream("teledyne.cfg", std::ifstream::in);
	std::string word;
	if(!cfgstream) return;

	while(std::getline(cfgstream,word,'=')) {
		settings.push_back(word);
		std::getline(cfgstream,word);
		values.push_back(word);
	}	
		
	cfgstream.close();
}
