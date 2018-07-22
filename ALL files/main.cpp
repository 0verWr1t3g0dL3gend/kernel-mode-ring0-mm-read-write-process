#include <Windows.h>
#include <winioctl.h>
#include <iostream>

#define KGETPID             CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KGETBASE_ADDRESS    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KREAD				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KWRITE				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef long long QWORD;

typedef struct DATA
{
	DWORD PID;
	QWORD Address;
	QWORD Buffer;
	SIZE_T Size;

} *PDATA;

int main()
{
	HANDLE hDriver = CreateFile(L"\\\\.\\GameHackL", GENERIC_ALL, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);
	bool choice;
	DATA Data;
	SecureZeroMemory(&Data, sizeof(Data));

	if (hDriver == INVALID_HANDLE_VALUE) std::cout << "Unable to open driver\n";

	else {
		std::cout << "PID: ";
		std::cin >> Data.PID;

		std::cout << "Address: ";
		std::cin >> Data.Address;

		std::cout << "Size: ";
		std::cin >> Data.Size;

		std::cout << "\nValue to write: ";
		std::cin >> Data.Buffer;

		std::cout << "1) Read\n2) Write\n";
		std::cin >> choice;

		if (choice==1)
		{
			DeviceIoControl(hDriver, KREAD, &Data, sizeof(Data), &Data, sizeof(Data), 0, 0);
			std::cout << "\n\nDATA READ: " << Data.Buffer << std::endl;
		}
		else
		{
			DeviceIoControl(hDriver, KWRITE, &Data, sizeof(Data), 0, 0, 0, 0);
		}

		CloseHandle(hDriver);
	}
	system("PAUSE");
	return 0;
}
