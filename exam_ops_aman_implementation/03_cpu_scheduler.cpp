#include <algorithm>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

using namespace std;

struct Job {
    int examId;
    int arrival;
    int burst;
};

struct Result {
    int examId;
    int arrival;
    int burst;
    int completion;
    int turnaround;
    int waiting;
};

void printResults(const string& title, const vector<Result>& results) {
    double totalWaiting = 0;
    double totalTurnaround = 0;

    cout << "\n" << title << "\n";
    cout << left << setw(12) << "Exam ID"
         << setw(12) << "Arrival"
         << setw(12) << "Burst"
         << setw(14) << "Completion"
         << setw(14) << "Turnaround"
         << setw(12) << "Waiting" << '\n';

    for (const auto& r : results) {
        cout << left << setw(12) << r.examId
             << setw(12) << r.arrival
             << setw(12) << r.burst
             << setw(14) << r.completion
             << setw(14) << r.turnaround
             << setw(12) << r.waiting << '\n';

        totalWaiting += r.waiting;
        totalTurnaround += r.turnaround;
    }

    if (!results.empty()) {
        cout << fixed << setprecision(2);
        cout << "Average waiting time: " << totalWaiting / results.size() << '\n';
        cout << "Average turnaround time: " << totalTurnaround / results.size() << '\n';
    }
}

vector<Result> runFCFS(vector<Job> jobs) {
    stable_sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        if (a.arrival != b.arrival) return a.arrival < b.arrival;
        return a.examId < b.examId;
    });

    vector<Result> results;
    int currentTime = 0;

    for (const auto& job : jobs) {
        currentTime = max(currentTime, job.arrival);
        currentTime += job.burst;

        int turnaround = currentTime - job.arrival;
        int waiting = turnaround - job.burst;

        results.push_back({job.examId, job.arrival, job.burst,
                           currentTime, turnaround, waiting});
    }

    return results;
}

vector<Result> runRoundRobin(const vector<Job>& jobs, int quantum) {
    vector<Job> sortedJobs = jobs;
    stable_sort(sortedJobs.begin(), sortedJobs.end(), [](const Job& a, const Job& b) {
        if (a.arrival != b.arrival) return a.arrival < b.arrival;
        return a.examId < b.examId;
    });

    const int n = static_cast<int>(sortedJobs.size());
    vector<int> remaining(n), completion(n, 0);
    queue<int> ready;
    int nextJob = 0;
    int currentTime = 0;

    for (int i = 0; i < n; ++i) remaining[i] = sortedJobs[i].burst;

    while (nextJob < n || !ready.empty()) {
        if (ready.empty() && nextJob < n && currentTime < sortedJobs[nextJob].arrival) {
            currentTime = sortedJobs[nextJob].arrival;
        }

        while (nextJob < n && sortedJobs[nextJob].arrival <= currentTime) {
            ready.push(nextJob);
            ++nextJob;
        }

        if (ready.empty()) continue;

        int index = ready.front();
        ready.pop();

        int runTime = min(quantum, remaining[index]);
        currentTime += runTime;
        remaining[index] -= runTime;

        while (nextJob < n && sortedJobs[nextJob].arrival <= currentTime) {
            ready.push(nextJob);
            ++nextJob;
        }

        if (remaining[index] > 0) {
            ready.push(index);
        } else {
            completion[index] = currentTime;
        }
    }

    vector<Result> results;
    for (int i = 0; i < n; ++i) {
        int turnaround = completion[i] - sortedJobs[i].arrival;
        int waiting = turnaround - sortedJobs[i].burst;
        results.push_back({sortedJobs[i].examId, sortedJobs[i].arrival,
                           sortedJobs[i].burst, completion[i], turnaround, waiting});
    }

    return results;
}

int main() {
    int n;
    cout << "Number of pending exam allocation jobs: ";
    cin >> n;

    if (!cin || n <= 0) {
        cout << "Enter a positive number of jobs.\n";
        return 1;
    }

    vector<Job> jobs(n);
    cout << "Enter Exam_ID Arrival_Time Burst_Time for each job:\n";

    for (auto& job : jobs) {
        cin >> job.examId >> job.arrival >> job.burst;
        if (!cin || job.arrival < 0 || job.burst <= 0) {
            cout << "Invalid input: arrival must be >= 0 and burst must be > 0.\n";
            return 1;
        }
    }

    int quantum;
    cout << "Enter Round Robin time quantum: ";
    cin >> quantum;

    if (!cin || quantum <= 0) {
        cout << "Time quantum must be positive.\n";
        return 1;
    }

    printResults("FCFS Scheduling", runFCFS(jobs));
    printResults("Round Robin Scheduling (quantum = " + to_string(quantum) + ")",
                 runRoundRobin(jobs, quantum));

    cout << "\nInterpretation:\n";
    cout << "- FCFS is simple and has low scheduling overhead, but a long job can delay later jobs.\n";
    cout << "- Round Robin gives each pending job a time slice; it is fairer for interactive workloads,\n";
    cout << "  but may add scheduling overhead. Choose based on the allocation workload and measurements.\n";
    cout << "- Burst time is an estimated processing duration for an allocation job, not actual wall-clock time.\n";

    return 0;
}
