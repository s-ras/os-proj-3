#include <time.h>
#include <windows.h>
#include <chrono>
#include <fstream>
#include <iostream>

using namespace std;

HANDLE threadReadWrite = CreateSemaphore(NULL, 1, 1, NULL);
HANDLE parentPrint = CreateSemaphore(NULL, 1, 1, NULL);
HANDLE parentDone = CreateSemaphore(NULL, 1, 1, NULL);
HANDLE activeThreadCount = CreateSemaphore(NULL, 1, 1, NULL);

int CORE_NUMBER;
int ACTIVE_THREAD = 0;

int DATA_COUNT;
long double* DATASET;
int TIME = -1;
long double OPTIMAL[3] = {0};

int WINNER_ID;
int* ANSWERS;
long double lastDiff = -1;

DWORD WINAPI calculate(void*);

int lineCount(fstream&);
void printResult(long double*, int*, int);
long double calculateDiff(int*, long double*, int, long double[]);

int main() {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	CORE_NUMBER = sysInfo.dwNumberOfProcessors;

	string FILE_NAME;
	fstream FILE;

	do {
		cout << "Dataset file name?\t";
		cin >> FILE_NAME;
		FILE.open("dataset/" + FILE_NAME + ".txt", ios::in);
		if (!FILE) {
			cout << "File not found, try again!" << endl;
		}
	} while (!FILE);

	cout << "------------------------------------" << endl;

	do {
		cout << "Time in seconds?\t";
		cin >> TIME;
		if (TIME <= 0) {
			cout << "Invalid time, try again!" << endl;
		}
	} while (TIME <= 0);

	cout << "------------------------------------" << endl;

	DATA_COUNT = lineCount(FILE);

	DATASET = new long double[DATA_COUNT];
	ANSWERS = new int[DATA_COUNT];

	cout << "Working on " << FILE_NAME << " with " << DATA_COUNT
		 << " records for " << TIME << " seconds:" << endl;

	for (int i = 0; i < DATA_COUNT; i++) {
		string read;
		getline(FILE, read, '\n');
		DATASET[i] = stof(read);
		cout << DATASET[i] << ' ';
	}

	cout << endl << "------------------------------------" << endl;

	cout << CORE_NUMBER << " CPU cores detected, launching " << CORE_NUMBER
		 << " threads" << endl;

	cout << "------------------------------------" << endl;

	long double total = 0;

	for (int i = 0; i < DATA_COUNT; i++) {
		total += DATASET[i];
	}

	OPTIMAL[0] = total * 2 / 5;
	OPTIMAL[1] = total * 2 / 5;
	OPTIMAL[2] = total * 1 / 5;

	int threadParams[CORE_NUMBER];
	HANDLE threadHandle[CORE_NUMBER];

	for (int i = 0; i < CORE_NUMBER; i++) {
		threadParams[i] = i;
		threadHandle[i] =
			CreateThread(NULL, 0, calculate, &threadParams[i], 0, NULL);
		ACTIVE_THREAD++;
		if (!threadHandle[i]) {
			cout << "Could not create thread, program will terminate." << endl;
			exit(1);
		}
	}

	while (true) {
		WaitForSingleObject(activeThreadCount, INFINITE);
		if (!ACTIVE_THREAD) {
			ReleaseSemaphore(activeThreadCount, 1, NULL);
			break;
		}
		ReleaseSemaphore(activeThreadCount, 1, NULL);
		WaitForSingleObject(parentPrint, INFINITE);
		cout << WINNER_ID << " Generated a new answer with diff = " << lastDiff
			 << endl;
		ReleaseSemaphore(parentDone, 1, NULL);
	}

	for (int i = 0; i < CORE_NUMBER; i++) {
		WaitForSingleObject(threadHandle[i], INFINITE);
		CloseHandle(threadHandle[i]);
	}

	cout << "------------------------------------" << endl;

	cout << "Winner is thread with ID " << WINNER_ID << " :" << endl;
	cout << "Diff : " << lastDiff << endl;

	cout << "------------------------------------" << endl;

	printResult(DATASET, ANSWERS, DATA_COUNT);

	delete ANSWERS;
	delete DATASET;

	CloseHandle(threadReadWrite);
	CloseHandle(parentPrint);
	CloseHandle(parentDone);
	CloseHandle(activeThreadCount);

	return 0;
}

int lineCount(fstream& F) {
	int res = 0;
	string read;
	while (getline(F, read)) {
		res++;
	};
	F.clear();
	F.seekg(0, ios::beg);
	return res;
}

long double calculateDiff(int* ans,
						  long double* data,
						  int size,
						  long double optimal[3]) {
	long double firstChild = 0;
	long double secondChild = 0;
	long double thirdChild = 0;

	for (int i = 0; i < size; i++) {
		switch (ans[i]) {
			case 0:
				firstChild += DATASET[i];
				break;
			case 1:
				secondChild += DATASET[i];
				break;
			case 2:
				thirdChild += DATASET[i];
				break;
			default:
				break;
		}
	}

	long double diff = abs(OPTIMAL[0] - firstChild) +
					   abs(OPTIMAL[1] - secondChild) +
					   abs(OPTIMAL[2] - thirdChild);

	return diff;
}

void printResult(long double* data, int* answers, int size) {
	cout << "First Child: " << endl;
	for (int i = 0; i < size; i++) {
		if (answers[i] == 0) {
			cout << data[i] << " ";
		}
	}
	cout << endl;

	cout << "Second Child: " << endl;
	for (int i = 0; i < size; i++) {
		if (answers[i] == 1) {
			cout << data[i] << " ";
		}
	}
	cout << endl;

	cout << "Third Child: " << endl;
	for (int i = 0; i < size; i++) {
		if (answers[i] == 2) {
			cout << data[i] << " ";
		}
	}
	cout << endl;
}

DWORD WINAPI calculate(void* param) {
	int* inp = static_cast<int*>(param);

	srand(time(0) + *inp);

	std::chrono::time_point startTime =
		std::chrono::high_resolution_clock::now();
	std::chrono::time_point currentTime = startTime;

	while (std::chrono::duration_cast<std::chrono::seconds>(currentTime -
															startTime)
			   .count() < TIME) {
		int random = rand() % 5;

		int newAnswers[DATA_COUNT];

		for (int i = 0; i < DATA_COUNT; i++) {
			int rnd = rand() % 5;
			switch (rnd) {
				case 0:
				case 1:
					newAnswers[i] = 0;
					break;
				case 2:
				case 3:
					newAnswers[i] = 1;
					break;
				case 4:
					newAnswers[i] = 2;
					break;
				default:
					break;
			}
		}

		long double newDiff =
			calculateDiff(newAnswers, DATASET, DATA_COUNT, OPTIMAL);
		if (newDiff < lastDiff || lastDiff == -1) {
			WaitForSingleObject(threadReadWrite, INFINITE);
			if (newDiff < lastDiff || lastDiff == -1) {
				WINNER_ID = *inp;
				for (int i = 0; i < DATA_COUNT; i++) {
					ANSWERS[i] = newAnswers[i];
				}
				lastDiff = newDiff;
				ReleaseSemaphore(parentPrint, 1, NULL);
				WaitForSingleObject(parentDone, INFINITE);
			}
			ReleaseSemaphore(threadReadWrite, 1, NULL);
		}

		currentTime = std::chrono::high_resolution_clock::now();
	}

	ReleaseSemaphore(parentPrint, 1, NULL);

	WaitForSingleObject(activeThreadCount, INFINITE);
	ACTIVE_THREAD--;
	ReleaseSemaphore(activeThreadCount, 1, NULL);

	return 0;
}