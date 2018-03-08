

// Used for memory-mapped functionality
#include <winsock2.h>
#include <Ws2tcpip.h>

#include <windows.h>
#include "sharedmemory.h"



// Used for this example


#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <cmath>
#include <sstream>



#pragma comment(lib, "Ws2_32.lib")

// Name of the pCars memory mapped file
#define MAP_OBJECT_NAME "$pcars2$"

float normalize(float val, int max){
	if (max <= 1 || val == 0.0) return val;
	float sign = 1;
	if (val < 0){
		val = -val;
		sign = -1;
	}
	if (val > max) val = max;
	return (float)log(val + 1) / log(max) * sign;
}

std::string Convert(float number) {
	std::ostringstream buff;
	buff << number;
	return buff.str();
}

int main()
{
	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	int wsOk = WSAStartup(version, &data);
	if (wsOk != 0) {
		printf_s("can't start winsock!");
		return 0;
	}

	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(10009);

	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

	SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);

	// Open the memory-mapped file
	HANDLE fileHandle = OpenFileMapping( PAGE_READONLY, FALSE, MAP_OBJECT_NAME );
	if (fileHandle == NULL)
	{
		printf( "Could not open file mapping object (%d).\n", GetLastError() );
		return 0;
	}

	// Get the data structure
	const SharedMemory* sharedData = (SharedMemory*)MapViewOfFile( fileHandle, PAGE_READONLY, 0, 0, sizeof(SharedMemory) );
	SharedMemory* localCopy = new SharedMemory;
	if (sharedData == NULL)
	{
		printf( "Could not map view of file (%d).\n", GetLastError() );

		CloseHandle( fileHandle );
		return 1;
	}

	// Ensure we're sync'd to the correct data version
	if ( sharedData->mVersion != SHARED_MEMORY_VERSION )
	{
		printf( "Data version mismatch\n");
		return 1;
	}
//	std::string inputComm = "command, enable, \n";
	
	//	sendto(out, inputComm.c_str(), inputComm.size() + 1, 0, (sockaddr*)&server, sizeof(server));

	//------------------------------------------------------------------------------
	// TEST DISPLAY CODE
	//------------------------------------------------------------------------------
	unsigned int updateIndex(0);
	unsigned int indexChange(0);
	printf( "ESC TO EXIT\n\n" );
	float maxValues[6] = { 1, 1, 1, 1, 1, 1 };
	float arr1[6];
	while (true)
	{
		if ( sharedData->mSequenceNumber % 2 )
		{
			// Odd sequence number indicates, that write into the shared memory is just happening
			continue;
		}

		indexChange = sharedData->mSequenceNumber - updateIndex;
		updateIndex = sharedData->mSequenceNumber;

		//Copy the whole structure before processing it, otherwise the risk of the game writing into it during processing is too high.
		memcpy(localCopy,sharedData,sizeof(SharedMemory));
		
		switch (localCopy->mGameState) {
		case 0:
			for (int i = 0; i < 6; i++) {
				maxValues[i] = 1;
			}
			break;
		}
		if (localCopy->mSequenceNumber != updateIndex )
		{
			// More writes had happened during the read. Should be rare, but can happen.
			continue;
		}
		printf("sequence number %d \n",localCopy->mSequenceNumber);

		printf( "mGameState: (%d)\n", localCopy->mGameState );
		printf( "mSessionState: (%d)\n", localCopy->mSessionState );
		printf("Speed: %f: \n", localCopy->mSpeed/ 0.44704);
		printf( "mOdometerKM: (%0.2f)\n", localCopy->mOdometerKM );

		printf("Orientation X : %f Y : %f Z : %f \n", localCopy->mOrientation[0], localCopy->mOrientation[1], localCopy->mOrientation[2]);
		printf("Local Velocity X : %f Y : %f Z : %f \n",localCopy->mLocalVelocity[0], localCopy->mLocalVelocity[1], localCopy->mLocalVelocity[2]);
		printf("World Velocity X : %f Y : %f Z : %f \n", localCopy->mWorldVelocity[0], localCopy->mWorldVelocity[1], localCopy->mWorldVelocity[2]);

		printf("Angular Velocity X : %f Y : %f Z : %f \n", localCopy->mAngularVelocity[0], localCopy->mAngularVelocity[1], localCopy->mAngularVelocity[2]);
		printf("Local Acceleration X : %f Y : %f Z : %f \n", localCopy->mLocalAcceleration[0], localCopy->mLocalAcceleration[1], localCopy->mLocalAcceleration[2]);
		printf("World Acceleration X : %f Y : %f Z : %f \n", localCopy->mWorldAcceleration[0], localCopy->mWorldAcceleration[1], localCopy->mWorldAcceleration[2]);
		printf("Extents Centre X : %f Y : %f Z : %f \n", localCopy->mExtentsCentre[0], localCopy->mExtentsCentre[1], localCopy->mExtentsCentre[2]);

		
		for (int i = 0; i < 6; i++)
		{
			switch (i) {
				case 0:
					arr1[i] = -localCopy->mLocalVelocity[1];//sent as x
					break;
				case 1:
					arr1[i] = localCopy->mLocalVelocity[0];//sent as y
					break;
				case 2:
					arr1[i] = localCopy->mLocalVelocity[2];//sent z
					break;
				case 3:
					arr1[i] = localCopy->mAngularVelocity[1];//sent as roll
					break;
				case 4:
					arr1[i] = localCopy->mAngularVelocity[0];//sent as pitch
					break;
				case 5:
					arr1[i] = localCopy->mAngularVelocity[2];//sent as yaw
					break;
			}
			float f = abs(arr1[i]);
			if (f > maxValues[i])
			{
				maxValues[i] = f;
			}
			arr1[i] = normalize(arr1[i], maxValues[i]);
		}
		
		std::string input = "xyzrpy,"+Convert(arr1[0]) + "," + Convert(arr1[1]) + "," + Convert(arr1[2]) +","+ Convert(arr1[3]) + "," + Convert(arr1[4]) + "," + Convert(arr1[5]) +",\n";
		
		sendto(out, input.c_str(), input.size() + 1, 0, (sockaddr*)&server, sizeof(server));
		
		
		
		
		

		system("cls");

		if ( _kbhit() && _getch() == 27 ) // check for escape
		{
			break;
		}
	}
	//------------------------------------------------------------------------------

	// Cleanup
	UnmapViewOfFile( sharedData );
	CloseHandle( fileHandle );
	delete localCopy;

	return 0;
}




