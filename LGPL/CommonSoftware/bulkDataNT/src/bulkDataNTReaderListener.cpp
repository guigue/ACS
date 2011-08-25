
#include "bulkDataNTReaderListener.h"
#include <iostream>
#include <iterator>

int BulkDataNTReaderListener::sleep_period=0;


BulkDataNTReaderListener::BulkDataNTReaderListener(const char* name, BulkDataCallback* cb)
:
				currentState_m(StartState),
				lost_packs(0),
				flowName_m(name),
				data_length(0),
				callback_m (cb)
{
	next_sample=0;
	message_dr=0;
}


BulkDataNTReaderListener::~BulkDataNTReaderListener ()
{
}

void BulkDataNTReaderListener::on_data_available(DDS::DataReader* reader)
{
	DDS::ReturnCode_t status;
	DDS::SampleInfo si ;
	ACSBulkData::BulkDataNTFrame message;
	unsigned char tmpArray[ACSBulkData::FRAME_MAX_LEN];


	if (message_dr==NULL)	message_dr = ACSBulkData::BulkDataNTFrameDataReader::narrow(reader);

	if  (  message_dr==NULL) {
		cerr << "read: _narrow failed." << endl;
		exit(1);
	}

	message.data.maximum(ACSBulkData::FRAME_MAX_LEN); //TBD constant from
	status = message_dr->take_next_sample(message, si) ;

	if (status == DDS::RETCODE_OK)
	{
		if (si.valid_data == 1)
		{
			switch(message.dataType)
			{
			case ACSBulkData::BD_PARAM:
			{
				cout << flowName_m << " startSend: parameter size: " << message.data.length() << endl;
				if (currentState_m==StartState || currentState_m==StopState)
				{
					data_length = 0;
					currentState_m = DataRcvState;
					message.data.to_array(tmpArray, message.data.length());
					callback_m->cbStart(tmpArray, message.data.length());
				}
				else
				{
					std::cerr << "Parameter arrived (BD_PARAM) in state: " << currentState_m << std::endl;
				}
				break;
			}//	case ACSBulkData::BD_PARAM:
			case ACSBulkData::BD_DATA:
			{
				if (currentState_m==DataRcvState)
				{
					if (data_length==0)
					{
						std::cout << " *************************   New sendData @ " << flowName_m << " *******************************" << std::endl;
						start_time = ACE_OS::gettimeofday();
					}

					data_length += message.data.length();

					if ( message.restDataLength>0)
					{
						if (next_sample!=0 && next_sample!=message.restDataLength)
						{
							cerr << flowName_m << "    " << ">>>> missed sample #: " << message.restDataLength << " for " << flowName_m << endl;
						}
						next_sample=message.restDataLength-1;
					}
					else //message.restDataLength==0
					{
						ACE_Time_Value elapsed_time = ACE_OS::gettimeofday() - start_time;
						cout <<	flowName_m << " Received all data from sendData: " << data_length << " Bytes in ";
						cout <<(elapsed_time.sec()+( elapsed_time.usec() / 1000000.0 ));
						cout << "secs. => Rate: ";
						cout << ((data_length/(1024.0*1024.0))/(elapsed_time.sec()+( elapsed_time.usec() / 1000000.0 ))) << "MBytes/sec" << endl;

						DDS::SampleLostStatus s;
						reader->get_sample_lost_status(s);
						cerr << flowName_m << " LOST samples: \t\t total_count: " << s.total_count << " total_count_change: " << s.total_count_change << endl;
						data_length = 0;
					}

					message.data.to_array(tmpArray, message.data.length());
					callback_m->cbReceive(tmpArray, message.data.length());
				}
				else
				{
					std::cerr << "Data of length " << message.data.length();
					std::cerr << " arrived (BD_DATA) in state: " << currentState_m << " on flow: " << flowName_m << std::endl;
				}
				break;
			}//case ACSBulkData::BD_DATA
			case ACSBulkData::BD_STOP:
			{
				if (currentState_m==DataRcvState)
				{
					currentState_m = StopState;
					cout << "===============================================================" << endl;
					callback_m->cbStop();
				}else
					if (currentState_m==StopState)
					{
						std::cerr << "Stop (BD_STOP) arrived in stop state - ignored!" << std::endl;
					}
					else //StartState
					{
						std::cerr << "Stop (BD_STOP) arrived in start state: no data has been sent after start!" << std::endl;
					}
				break;
			}//case ACSBulkData::BD_STOP
			default:
				cerr << "Unknown message.dataType: " << message.dataType << endl;
			}//switch
		}else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
		{
			cout << "instance is disposed" << endl;
		}
		else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
		{
			cout << "instance is unregistered" << endl;
		}
		else
		{
			cout << "BulkDataNTReaderListener(" << flowName_m << ")::on_data_available:";
			cout << " received unknown instance state " << si.instance_state;
			cout << endl;
		}
	}else if (status == DDS::RETCODE_NO_DATA) {
		cerr << "ERROR: on_data_available DDS::RETCODE_NO_DATA!" << endl;
	}
	else
	{
		cerr << "ERROR: on_data_available status: " << status << endl;
	}//if(status)
}//on_data_available

void BulkDataNTReaderListener::on_requested_deadline_missed (
		DDS::DataReader*,
		const DDS::RequestedDeadlineMissedStatus& )
{
	cerr << "BulkDataNTReaderListener(" << flowName_m << ")::on_requested_deadline_missed" << endl;
}

void BulkDataNTReaderListener::on_requested_incompatible_qos (
		DDS::DataReader*,
		const DDS::RequestedIncompatibleQosStatus&)
{
	cerr << "BulkDataNTReaderListener(" << flowName_m << ")::on_requested_incompatible_qos" << endl;
}

void BulkDataNTReaderListener::on_liveliness_changed (
		DDS::DataReader*,
		const DDS::LivelinessChangedStatus& lcs)
{
	if (lcs.alive_count_change>0)
	{
		cout << "A new sender has connected to flow: " << callback_m->getFlowName();
		cout << " on stream: " << callback_m->getStreamName() << endl;
	}else
	{
		cout << "A sender has disconnected from flow: " << callback_m->getFlowName();
		cout << " on stream: " << callback_m->getStreamName() << endl;
	}

	//	cerr << "BulkDataNTReaderListener(" << listName << ")::on_liveliness_changed:" << endl;
	//	cerr << "    alive_count: " << lcs.alive_count << endl;
	//	cerr << "    not_alive_count: " << lcs.not_alive_count << endl;
	//	cerr << "    alive_count_change: " << lcs.alive_count_change << endl;
	//	cerr << "    not_alive_count_change: " << lcs.not_alive_count_change << endl;
	//
	//	cout << "Received:"  << data_length << endl;
}

void BulkDataNTReaderListener::on_subscription_matched (
		DDS::DataReader*,
		const DDS::SubscriptionMatchedStatus& )
{
	cerr << "BulkDataNTReaderListener(" << flowName_m << ")::on_subscription_match" << endl;
	lost_packs = 0;
}

void BulkDataNTReaderListener::on_sample_rejected(
		DDS::DataReader*,
		const DDS::SampleRejectedStatus& srs)
{
	cerr << "BulkDataNTReaderListener(" << flowName_m << "::on_sample_rejected SampleRejectedStatus.last_reason: ";
	cerr << srs.last_reason << " SampleRejectedStatus.total_count_change: " << srs.total_count_change;
	cerr << " SampleRejectedStatus.total_count: " << srs.total_count << endl;
}

void BulkDataNTReaderListener::on_sample_lost(
		DDS::DataReader*,
		const DDS::SampleLostStatus& s)
{
	cerr << endl << endl << "BulkDataNTReaderListener(" << flowName_m << "::on_sample_lost: ";
	cerr << "total_count: " << s.total_count << " total_count_change: " << s.total_count_change << endl << endl;
}
