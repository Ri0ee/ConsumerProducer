#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <string>
#include <vector>
#include <chrono>
using namespace std;

class Buffer {
public:

	void Push(const string& string_) {
		unique_lock<mutex> locker(mu);
		while (m_data.size() >= 10) cond.wait(locker);

		m_data.push_back(string_);

		locker.unlock();
		cond.notify_all();
	}

	std::string Pop() {
		unique_lock<mutex> locker(mu);
		while (m_data.empty()) cond.wait(locker);

		string ret = m_data.front();
		m_data.pop_front();

		locker.unlock();
		cond.notify_all();
		return ret;
	}

	bool ShouldFinish() {
		return m_end_of_data && m_data.empty();
	}

	void SetEndOfData() {
		m_end_of_data = true;
	}

private:
	deque<string> m_data;
	bool m_end_of_data = false;

	mutex mu;
	condition_variable cond;
};

class Consumer {
public:
	Consumer(Buffer& buffer_) : m_buffer(buffer_) {}
	~Consumer() {
		int thread_count = m_threads.size();
		for (int i = 0; i < thread_count; i++) {
			m_threads[i]->join();
			delete m_threads[i];
		}
	}

	void Run(int thread_count_) {
		for (int i = 0; i < thread_count_; i++)
			m_threads.push_back(new thread(&Consumer::Tick, this));
	}

	void Tick() {
		string data;
		while (!m_buffer.ShouldFinish()) {
			data = m_buffer.Pop();

			lock_guard<mutex> locker(cout_mu);
			std::cout << data << "\n";
		}
	}

private:
	Buffer& m_buffer;
	vector<thread*> m_threads;
	mutex cout_mu;
};

class Producer {
public:
	Producer(Buffer& buffer_) : m_buffer(buffer_) {}
	~Producer() {
		int thread_count = m_threads.size();
		for (int i = 0; i < thread_count; i++) {
			m_threads[i]->join();
			delete m_threads[i];
		}
	}

	void Run(int thread_count_) {
		for (int i = 0; i < thread_count_; i++)
			m_threads.push_back(new thread(&Producer::Tick, this));
	}

	void Tick() {
		for (int i = 0; i < 10000; i++)
			m_buffer.Push(to_string(i));

		m_buffer.SetEndOfData();
	}

private:
	Buffer& m_buffer;
	vector<thread*> m_threads;
};

int main() {
	Buffer buf;
	Producer* prod = new Producer(buf);
	Consumer* cons = new Consumer(buf);

	prod->Run(1);
	cons->Run(3);

	delete prod;
	delete cons;

	system("PAUSE");
	return 0;
}