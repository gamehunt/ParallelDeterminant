#include <atomic>
#include <cassert>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#define PERF_START(id) \
    auto start_##id = std::chrono::high_resolution_clock::now();

#define PERF_END(id) \
    auto end_##id = std::chrono::high_resolution_clock::now();

#define PERF_RESULT(id) \
    std::chrono::duration_cast<std::chrono::microseconds>(end_##id - start_##id).count()

typedef std::vector<std::vector<int>> matrix_data;

class Mat {
public:
    Mat() {
        n = 0;

        parent = nullptr;
        ar = 0;
        ac = 0;
    }

    Mat(int _n): Mat() {
        n = _n;
        rows.resize(n);
        for(int i = 0; i < n; i++) {
            rows[i].resize(n);
        }
    }

    Mat(Mat& other): Mat(other.size()) {
        for(int i = 0; i < size(); i++) {
            for(int j = 0; j < size(); j++) {
                rows[i][j] = other[i][j];
            }
        }
    }

    Mat(const Mat* parent, int ar, int ac): n(parent->size() - 1), parent(parent), ar(ar), ac(ac) {}

    virtual ~Mat() {}

    bool is_valid_pos(int r, int c) const {
        return r >= 0 && c >= 0 && r < n && c < n;
    }

    int get(int r, int c) const {
        if(!is_valid_pos(r, c)) {
            throw std::out_of_range("invalid pos");
        } 
        if(!parent) {
            return rows[r][c];
        } else {
            if(r >= ar) {
                r += 1;
            }
            if(c >= ac) {
                c += 1;
            }
            return parent->get(r, c);
        }
    }

    void set(int r, int c, int v) {
        if(!is_valid_pos(r, c)) {
            throw std::out_of_range("invalid pos");
        } 
        if(!parent) {
            rows[r][c] = v;
        } else {
            throw std::runtime_error("Read only");
        }
    }

    std::vector<int>& operator[](int i) {
        if(i < 0 || i >= n) {
            throw std::out_of_range("invalid pos");
        } 
        if(!parent) {
            return rows[i];
        } else {
            throw std::runtime_error("Read only");
        }
    }

    std::vector<int> operator[](int i) const {
        if(i < 0 || i >= n) {
            throw std::out_of_range("invalid pos");
        } 
        if(!parent) {
            return rows[i];
        } else {
            if(i >= ar) {
                i++;
            }
            return parent->rows[i];
        }
    }

    friend std::ostream& operator<<(std::ostream& stream, const Mat& matrix) {
        for(int i = 0; i < matrix.rows.size(); i++) {
            for(int j = 0; j < matrix.rows.size(); j++) {
                stream << matrix.get(i, j) << " ";
            }
            stream << "\n";
        }
        return stream;
    }

    int size() const {
        return n;
    }

    int det() const {
        int sz = size();
        if(sz == 1) {
            return get(0, 0);
        } else {
            int r = 0;
            for(int i = 0; i < sz; i++) {
                r += (i % 2 ? -1 : 1) * get(i, 0) * minor(i, 0).det();
            }
            return r;
        }
    }

    int det_parallel() const {
        int sz = size();
        if(sz == 1) {
            return get(0, 0);
        } else {
            std::atomic<int> r = 0;
            std::vector<std::thread> threads;
            for(int i = 0; i < sz; i++) {
                threads.emplace_back(
                    std::thread([this, i, sz, &r] {
                        int _r = (i % 2 ? -1 : 1) * get(i, 0) * minor(i, 0).det();
                        r.fetch_add(_r);
                    })
                );
            }

            for(auto& t : threads) {
                t.join();
            }

            return r;
        }
    }

    Mat minor(int r, int c) const {
        return Mat(this, r, c);
    }

private:
    int n;
    matrix_data rows;

    const Mat* parent;
    int ar, ac;
};

int main() {
    std::ifstream fin("in.txt");

    if(!fin.is_open()) {
        std::cout << "Failed to open in.txt." << std::endl;
        return 1;
    }

    int n;
    fin >> n;

    Mat input(n);

    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            int v;
            fin >> v;
            input[i][j] = v;
        }
    }

    std::cout << "Matrix:" << std::endl;
    std::cout << input;

    PERF_START(serial)
    int s = input.det();
    PERF_END(serial)

    PERF_START(par)
    int p = input.det_parallel();
    PERF_END(par)

    assert(s == p);

    std::cout << "Determinant serial: " << s << std::endl;
    std::cout << "Took " << PERF_RESULT(serial) << "mcs" << std::endl;
    std::cout << "Determinant parallel: " << p << std::endl;
    std::cout << "Took " << PERF_RESULT(par) << "mcs" << std::endl;

    return 0;
}
