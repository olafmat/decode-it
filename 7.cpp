#include <stdlib.h>
#include <memory.h>
#include <iostream>

#define MAX_WIDTH 50
#define MAX_HEIGHT 50
#define MAX_HEIGHT2 64

using namespace std;

struct Shape;
struct ShapeList;

struct Board {
    int w, h;
    char board[MAX_WIDTH][MAX_HEIGHT2];

    void operator= (const Board& src) {
        w = src.w;
        h = src.h;
        for (int j = 0; j < w; j++) {
            memcpy(board[j], src.board[j], h);
        }
    }

    static Board* randomBoard(int w, int h, int c) {
        Board* board = new Board();
        board->w = w;
        board->h = h;
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                board->board[j][i] = (random() % c) + 1;
            }
        }
        return board;
    }

    void print() {
        for (int i = h - 1; i >= 0; i--) {
            for (int j = 0; j < w; j++) {
                cout << (board[j][i] ? char('A' + board[j][i]) : '-');
            }
            cout << endl;
        }
    }

    void remove(Shape *shape) {

    }
};

struct Shape {
    int minX, maxX;
    int y;
    int size;
    char c;

    int score() {
        return size * (size - 1);
    }

    void print() {
        cout << "minX=" << minX << " maxX=" << maxX << " y=" << y << " size=" << size << " c=" << char('A' + c) << " score=" << score() << endl;
    }
};

uint64_t used[MAX_WIDTH];

struct ShapeList {
    int size;
    Shape shapes[MAX_WIDTH * MAX_HEIGHT];

    ShapeList(): size(0) {
    }

    int addPoint(Board* board, int x, int y, Shape* dest) {
        const uint64_t mask = uint64_t(1) << y;
        if ((used[x] & mask) == uint64_t(0) && board->board[x][y] == dest->c) {
            used[x] |= mask;
            if (x < dest->minX) {
                dest->minX = x;
                dest->y = y;
            }
            if (x > dest->maxX) {
                dest->maxX = x;
            }
            return 1
                + (x > 0 ? addPoint(board, x - 1, y, dest) : 0)
                + (x < board->w - 1 ? addPoint(board, x + 1, y, dest) : 0)
                + (y > 0 ? addPoint(board, x, y - 1, dest) : 0)
                + (y < board->h - 1 ? addPoint(board, x, y + 1, dest) : 0);
        }
        return 0;
    }

    void addShape(Board* board, int x, int y, Shape* dest) {
        dest->minX = board->w;
        dest->maxX = -1;
        dest->c = board->board[x][y];
        dest->size = addPoint(board, x, y, dest);
    }

    void update(Board *board, int minX = 0, int maxX = MAX_WIDTH) {
        if (minX < 0) {
            minX = 0;
        }
        const int w = board->w;
        const int h = board->h;
        if (maxX >= w) {
            maxX = w - 1;
        }

        Shape* src = shapes;
        Shape* dest = shapes;
        for (int i = 0; i < size; i++) {
            if (src->maxX < minX || src->minX > maxX) {
                if (dest != src) {
                    *dest = *src;
                }
                dest++;
            }
            src++;
        }

        memset(used, 0, sizeof(uint64_t) * w);
        uint64_t* usedCol = used;
        for (int x = minX; x <= maxX; x++) {
            char* col = board->board[x];
            for (int y = 0; y < h; y++) {
                if (col[y] && !((*usedCol >> y) & 1)) {
                    addShape(board, x, y, dest);
                    if (dest->size > 1) {
                        dest++;
                    }
                }
            }
            usedCol++;
        }
        size = dest - shapes;
    }

    void operator= (const ShapeList& src) {
        size = src.size;
        memcpy(shapes, src.shapes, sizeof(shapes[0]) * size);
    }

    int score() {
        int total = 0;
        for (int i = 0; i < size; i++) {
            total += shapes[i].score();
        }
        return total;
    }

    void print() {
        for (int i = 0; i < size; i++) {
            shapes[i].print();
        }
        cout << "score: " << score() << endl;
    }
};

int main() {
    Board* board = Board::randomBoard(8, 8, 7);
    board->print();

    ShapeList list;
    /*for (int i = 0; i < 1000000; i++) {
        list.update(board);
    }*/
    list.update(board);
    list.print();

    cout << endl;
    ShapeList list2;
    list2.update(board, 2, 3);
    list2.update(board, 6, 6);
    list2.update(board, 7, 7);
    list2.update(board, 0, 0);
    list2.update(board, 3, 5);
    list2.update(board, 1, 1);
    list2.update(board, 4, 5);
    list2.print();
    return 0;
}
