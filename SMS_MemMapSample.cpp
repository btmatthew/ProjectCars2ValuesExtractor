

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



#pragma comment(lib, "Ws2_32.lib")

// Name of the pCars memory mapped file
#define MAP_OBJECT_NAME "$pcars2$"

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
	server.sin_port = htons(54000);

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


	//------------------------------------------------------------------------------
	// TEST DISPLAY CODE
	//------------------------------------------------------------------------------
	unsigned int updateIndex(0);
	unsigned int indexChange(0);
	printf( "ESC TO EXIT\n\n" );
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


		if (localCopy->mSequenceNumber != updateIndex )
		{
			// More writes had happened during the read. Should be rare, but can happen.
			continue;
		}

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

		//std::string a = std::to_string(localCopy->mOrientation[0])+","+ std::to_string(localCopy->mOrientation[1])+","+ std::to_string(localCopy->mOrientation[2]);
		
		
		std::string input = std::to_string(localCopy->mLocalAcceleration[0]) + "," + std::to_string(localCopy->mLocalAcceleration[1]) + "," + std::to_string(localCopy->mLocalAcceleration[2])+","+std::to_string(localCopy->mAngularVelocity[0]) + "," + std::to_string(localCopy->mAngularVelocity[1]) + "," + std::to_string(localCopy->mAngularVelocity[2])+",\n";

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
