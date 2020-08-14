#include <stdlib.h>
#include <memory.h>
#include <iostream>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "dirent.h"

#define MAX_WIDTH 52
#define MAX_HEIGHT 52
#define MAX_HEIGHT2 64
#define MAX_COLOR 20
//#define USE_RAND
//#define USE_ELECTIONS
//#define VALIDATION

#pragma pack(1)

using namespace std;

struct Shape;
struct ShapeList;

uint64_t used[MAX_WIDTH];

struct Move {
    int8_t x, y;
};

struct Shape {
    int8_t minX, maxX, minY, maxY;
    int16_t size;
    char c;
    int8_t y;
    char vs;
    #ifdef USE_RAND
    char rand;
    #endif

    int score() {
        return size * (size - 1);
    }

    void print() {
        cout << "minX=" << (int)minX << " maxX=" << (int)maxX << " minY=" << (int)minY << " maxY=" << (int)maxY <<
            " y=" << (int)y << " size=" << (int)size << " c=" << char('A' + c) << " score=" << score() << endl;
    }
};

struct Board {
    int w, h;
    char board[MAX_WIDTH][MAX_HEIGHT2];

    #ifdef USE_RAND
    int size[MAX_WIDTH];
    #endif

    void operator= (const Board& src) {
        w = src.w;
        h = src.h;
        for (int x = 0; x <= w + 1; x++) {
            memcpy(board[x], src.board[x], h + 2);
        }
    }

    void addFrame() {
        for (int x = 0; x <= w + 1; x++) {
            board[x][0] = board[x][h + 1] = 0;
        }
        for (int y = 1; y <= h; y++) {
            board[0][y] = board[w + 1][y] = 0;
        }
    }

    void loadFromCin() {
        int nc;
        cin >> h >> w >> nc;
        addFrame();
        for (int y = h; y > 0; y--) {
            for (int x = 1; x <= w; x++) {
                int c;
                cin >> c;
                board[x][y] = c + 1;
            }
        }
    }

    static Board* randomBoard(int w, int h, int c) {
        Board* board = new Board();
        board->w = w;
        board->h = h;
        board->addFrame();
        for (int y = 1; y <= h; y++) {
            for (int x = 1; x <= w; x++) {
                board->board[x][y] = (random() % c) + 1;
            }
        }
        return board;
    }

    void print() {
        for (int y = h; y > 0; y--) {
            for (int x = 1; x <= w; x++) {
                cout << (board[x][y] ? char('A' + board[x][y]) : '-');
            }
            cout << endl;
        }
    }

    void addSegment(int x, int minY, int maxY1, bool left, bool right, char c) {
        char *col = board[x];
        int nminY = minY;
        if (col[nminY] == c) {
            char* col1 = col - 1;
            while (col1[nminY] == c) {
                nminY--;
            }
        } else {
            do {
                nminY++;
                if (nminY >= maxY1) {
                    return;
                }
            } while(col[nminY] != c);
        }

        int nmaxY1 = nminY;
        do {
            nmaxY1++;
        } while(col[nmaxY1] == c);

        if (maxY1 - nmaxY1 >= 2) {
            addSegment(x, nmaxY1 + 1, maxY1, left, right, c);
        }

        uint64_t mask = (uint64_t(1) << nmaxY1) - (uint64_t(1) << nminY);
        if ((used[x] & mask) != uint64_t(0)) {
            return;
        }
        used[x] |= mask;

        if (x > 1 && left) {
            addSegment(x - 1, nminY, nmaxY1, true, false, c);
        }
        if (x < w && right) {
            addSegment(x + 1, nminY, nmaxY1, false, true, c);
        }
        if (nmaxY1 - maxY1 >= 2) {
            if (x > 1 && !left) {
                addSegment(x - 1, maxY1 + 1, nmaxY1, true, false, c);
            }
            if (x < w && !right) {
                addSegment(x + 1, maxY1 + 1, nmaxY1, false, true, c);
            }
        }
        if (minY - nminY >= 2) {
            if (x > 1 && !left) {
                addSegment(x - 1, nminY, minY - 1, true, false, c);
            }
            if (x < w && !right) {
                addSegment(x + 1, nminY, minY - 1, false, true, c);
            }
        }
    }

    /*void addPoint(int x, int y, char c) {
        const uint64_t mask = uint64_t(1) << y;
        if ((used[x] & mask) == uint64_t(0) && board[x][y] == c) {
            used[x] |= mask;
            addPoint(x - 1, y, c);
            addPoint(x + 1, y, c);
            addPoint(x, y - 1, c);
            addPoint(x, y + 1, c);
        }
    }*/

    void remove(Shape& shape) {
        memset(used + shape.minX, 0, sizeof(uint64_t) * (shape.maxX - shape.minX + 1));
        addSegment2(shape.minX, shape.y, shape.y + 1, true, true, shape.c);
        //addPoint(shape.minX, shape.y, shape.c);
        for (int x = shape.minX; x <= shape.maxX; x++) {
            uint64_t mask = used[x] >> shape.minY;
            char* src = board[x] + shape.minY;
            char* dest = src;
            for (int y = shape.minY; y <= h; y++) {
                if ((mask & 1) == uint64_t(0)) {
                    if (src != dest) {
                        if (!*dest) {
                            break;
                        }
                        *dest = *src;
                    }
                    dest++;
                }
                src++;
                mask >>= 1;
            }
            while(dest != src) {
                *dest = 0;
                dest++;
            }
        }
    }

    int addSegment2(int x, int minY, int maxY1, bool left, bool right, char c) {
        char *col = board[x];
        int nminY = minY;
        if (col[nminY] == c) {
            char* col1 = col - 1;
            while (col1[nminY] == c) {
                nminY--;
            }
        } else {
            do {
                nminY++;
                if (nminY >= maxY1) {
                    return 0;
                }
            } while(col[nminY] != c);
        }

        int nmaxY1 = nminY;
        do {
            nmaxY1++;
        } while(col[nmaxY1] == c);

        int total = 0;
        if (maxY1 - nmaxY1 >= 2) {
            total += addSegment2(x, nmaxY1 + 1, maxY1, left, right, c);
        }

        uint64_t mask = (uint64_t(1) << nmaxY1) - (uint64_t(1) << nminY);
        if ((used[x] & mask) != uint64_t(0)) {
            return total;
        }
        used[x] |= mask;

        total += nmaxY1 - nminY;
        if (x > 1 && left) {
            total += addSegment2(x - 1, nminY, nmaxY1, true, false, c);
        }
        if (x < w && right) {
            total += addSegment2(x + 1, nminY, nmaxY1, false, true, c);
        }
        if (nmaxY1 - maxY1 >= 2) {
            if (x > 1 && !left) {
                total += addSegment2(x - 1, maxY1 + 1, nmaxY1, true, false, c);
            }
            if (x < w && !right) {
                total += addSegment2(x + 1, maxY1 + 1, nmaxY1, false, true, c);
            }
        }
        if (minY - nminY >= 2) {
            if (x > 1 && !left) {
                total += addSegment2(x - 1, nminY, minY - 1, true, false, c);
            }
            if (x < w && !right) {
                total += addSegment2(x + 1, nminY, minY - 1, false, true, c);
            }
        }
        return total;
    }

    /*int addPoint2(int x, int y, char c) {
        const uint64_t mask = uint64_t(1) << y;
        int total = 0;
        if ((used[x] & mask) == uint64_t(0) && board[x][y] == c) {
            used[x] |= mask;
            total++;
            total += addPoint2(x - 1, y, c);
            total += addPoint2(x + 1, y, c);
            total += addPoint2(x, y - 1, c);
            total += addPoint2(x, y + 1, c);
        }
        return total;
    }*/

    int remove2(Move move) {
        memset(used, 0, sizeof(uint64_t) * (w + 2));
        int size = addSegment2(move.x, move.y, move.y + 1, true, true, board[move.x][move.y]); //addPoint2(move.x, move.y, board[move.x][move.y]);
        if (!size) {
            return 0;
        }
        int x = 1;
        while(used[x] == uint64_t(0)) {
            x++;
        }
        for (; x <= w; x++) {
            uint64_t mask = used[x];
            if (mask == uint64_t(0)) {
                break;
            }
            char* src = board[x] + 1;
            char* dest = src;
            for (int y = 1; y <= h; y++) {
                if ((mask & 2) == uint64_t(0)) {
                    if (src != dest)
                        *dest = *src;
                    dest++;
                }
                src++;
                mask >>= 1;
            }
            while(dest != src) {
                *dest = 0;
                dest++;
            }
        }
        return size * (size - 1);
    }
};

struct ShapeList {
    int size;
    Shape shapes[MAX_WIDTH * MAX_HEIGHT];

    ShapeList(): size(0) {
    }

    void operator=(const ShapeList& src) {
        size = src.size;
        memcpy(shapes, src.shapes, size * sizeof(Shape));
    }

    int addSegment(Board* board, int x, int minY, int maxY1, bool left, bool right, Shape* dest) {
        char c = dest->c;
        char *col = board->board[x];
        int nminY = minY;
        if (col[nminY] == c) {
            char* col1 = col - 1;
            while (col1[nminY] == c) {
                nminY--;
            }
        } else {
            do {
                nminY++;
                if (nminY >= maxY1) {
                    return 0;
                }
            } while(col[nminY] != c);
        }

        int nmaxY1 = nminY;
        do {
            nmaxY1++;
        } while(col[nmaxY1] == c);

        int total = 0;
        if (maxY1 - nmaxY1 >= 2) {
            total += addSegment(board, x, nmaxY1 + 1, maxY1, left, right, dest);
        }

        uint64_t mask = (uint64_t(1) << nmaxY1) - (uint64_t(1) << nminY);
        if ((used[x] & mask) != uint64_t(0)) {
            return total;
        }
        used[x] |= mask;

        if (left && x < dest->minX) {
            dest->minX = x;
            dest->y = nminY;
        }
        if (right && x > dest->maxX) {
            dest->maxX = x;
        }
        if (nminY < dest->minY) {
            dest->minY = nminY;
        }
        if (nmaxY1 - 1 > dest->maxY) {
            dest->maxY = nmaxY1 - 1;
        }

        total += nmaxY1 - nminY;
        if (x > 1 && left) {
            total += addSegment(board, x - 1, nminY, nmaxY1, true, false, dest);
        }
        if (x < board->w && right) {
            total += addSegment(board, x + 1, nminY, nmaxY1, false, true, dest);
        }
        if (nmaxY1 - maxY1 >= 2) {
            if (x > 1 && !left) {
                total += addSegment(board, x - 1, maxY1 + 1, nmaxY1, true, false, dest);
            }
            if (x < board->w && !right) {
                total += addSegment(board, x + 1, maxY1 + 1, nmaxY1, false, true, dest);
            }
        }
        if (minY - nminY >= 2) {
            if (x > 1 && !left) {
                total += addSegment(board, x - 1, nminY, minY - 1, true, false, dest);
            }
            if (x < board->w && !right) {
                total += addSegment(board, x + 1, nminY, minY - 1, false, true, dest);
            }
        }
        return total;
    }

    /*int addPoint(Board* board, int x, int y, Shape* dest) {
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
            if (y < dest->minY) {
                dest->minY = y;
            }
            if (y > dest->maxY) {
                dest->maxY = y;
            }
            return 1
                + addPoint(board, x - 1, y, dest)
                + addPoint(board, x + 1, y, dest)
                + addPoint(board, x, y - 1, dest)
                + addPoint(board, x, y + 1, dest);
        }
        return 0;
    }*/

    void addShape(Board* board, int x, int y, Shape* dest) {
        dest->minX = board->w + 1;
        dest->maxX = -1;
        dest->minY = board->h + 1;
        dest->maxY = -1;
        dest->c = board->board[x][y];
        dest->size = addSegment(board, x, y, y + 1, true, true, dest); //addPoint(board, x, y, dest);
        #ifdef VALIDATION
        if (dest->size <= 1) {
            cout << "INV " << x << " " << y << endl;
            dest->print();
            board->print();
        }
        #endif
        dest->vs = (dest->maxX == dest->minX && !board->board[dest->minX][dest->maxY + 1])/* ||
            !(dest->maxX == dest->minX &&
                board->board[dest->minX][dest->minY - 1] == board->board[dest->minX][dest->maxY + 1])*/;
        #ifdef USE_RAND
        dest->rand = random();
        #endif
    }

    void update(Board *board, int minX = 0, int maxX = MAX_WIDTH, int minY = 0) {
        if (minX < 0) {
            minX = 0;
        }
        const int w = board->w;
        const int h = board->h;
        if (maxX > w) {
            maxX = w;
        }
        if (minY < 1) {
            minY = 1;
        }

        Shape* src = shapes;
        //Shape* dest = shapes + size;
        for (int i = 0; i < size; i++) {
            /*if (src->maxX < minX || src->minX > maxX || src->maxY < minY) {
                if (dest != src) {
                    //cout << __LINE__ << " " << src-shapes << " " << dest-shapes << endl;
                    *dest = *src;
                }
                dest++;
            }*/
            if (src->maxX >= minX && src->minX <= maxX /*&& src->maxY >= minY*/) {
                //src->print();
                size--;
                *src = shapes[size];
                i--;
                continue;
            }
            src++;
        }
        Shape* dest = shapes + size;

        memset(used, 0, sizeof(uint64_t) * (w + 2));
        uint64_t* usedCol = used + minX;

        for (int x = minX; x <= maxX; x++) {
            char* pcol = &board->board[x - 1][0/*minY*/];
            char* col = &board->board[x][1/*minY*/];
            char* ncol = &board->board[x + 1][0/*minY*/];
            for (int y = 1/*minY*/; y <= h; y++) {
                char c = *col;
                if (!c) {
                    break;
                }
                if (!((*usedCol >> y) & 1) &&
                (pcol[y] == c || ncol[y] == c ||
                 col[-1] == c || col[1] == c))  {
                    addShape(board, x, y, dest);
                    dest++;
                }
                col++;
            }
            usedCol++;
        }
        size = dest - shapes;
    }

    long score() {
        long total = 0;
        for (int i = 0; i < size; i++) {
            total += shapes[i].score();
        }
        return total;
    }

    Shape& findBest(int (*comparator)(const Shape*, const Shape*)) {
        Shape* best = &shapes[0];
        for (int i = 1; i < size; i++) {
            if (comparator(&shapes[i], best) < 0) {
                best = &shapes[i];
            }
        }
        return *best;
    }

    /*void sort(int (*comparator)(const void*, const void*)) {
        qsort(shapes, size, sizeof(shapes[0]), comparator);
    }*/

    void print() {
        for (int i = 0; i < size; i++) {
            shapes[i].print();
        }
        cout << "score: " << score() << endl;
    }
};

struct Game {
    long total;
    int nmoves;
    Move moves[MAX_WIDTH * MAX_HEIGHT];

    void reset() {
        total = 0;
        nmoves = 0;
    }

    void operator=(const Game& src) {
        total = src.total;
        nmoves = src.nmoves;
        memcpy(moves, src.moves, nmoves * sizeof(Move));
    }

    void move(Shape &shape) {
        total += shape.score();
        #ifdef VALIDATION
        if (total > 6250000) {
            cout << "TOTAL " << total << endl;
            shape.print();
        }
        #endif
        moves[nmoves].x = shape.minX;
        moves[nmoves].y = shape.y;
        nmoves++;
    }

    void move(Move move, int score) {
        total += score;
        moves[nmoves].x = move.x;
        moves[nmoves].y = move.y;
        nmoves++;
    }

    void send(int height) {
        cout << "Y" << endl;
        for (int i = 0; i < nmoves; i++) {
            cout << (height - moves[i].y) << " " << (moves[i].x - 1) << endl;
        }
        cout << "-1 -1" << endl;
    }
};

int fromSmallest(const Shape *a, const Shape *b) {
    return a->size - b->size;
}

int fromLargest(const Shape *a, const Shape *b) {
    return b->size - a->size;
}

int fromTop(const Shape *a, const Shape *b) {
    return b->y - a->y;
}

int fromBottom(const Shape *a, const Shape *b) {
    return a->y - b->y;
}

int fromTop2(const Shape *a, const Shape *b) {
    return b->minY - a->minY;
}

int fromBottom2(const Shape *a, const Shape *b) {
    return a->minY - b->minY;
}

int fromTop3(const Shape *a, const Shape *b) {
    return b->maxY - a->maxY;
}

int fromBottom3(const Shape *a, const Shape *b) {
    return a->maxY - b->maxY;
}

int fromLeft(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    return a->minX - b->minX;
}

template<int chosenOne> int fromSmallestWithoutOne(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca - cb;
    }
    return a->size - b->size;
}

template<int chosenOne> int fromTopWithoutOne(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca - cb;
    }
    return b->y - a->y;
}

int colorHistogram[MAX_COLOR + 1];
char mostPopularColor;

int fromSmallestWithoutMostPop(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    int ca = a->c == mostPopularColor;
    int cb = b->c == mostPopularColor;
    if (ca != cb) {
        return ca - cb;
    }
    return a->size - b->size;
}

int fromLargestWithoutMostPop(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    int ca = a->c == mostPopularColor;
    int cb = b->c == mostPopularColor;
    if (ca != cb) {
        return ca - cb;
    }
    return b->size - a->size;
}

int byColorAndFromSmallest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return colorHistogram[a->c] - colorHistogram[b->c];
    }
    return a->size - b->size;
}

int byColorAndFromLargest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return colorHistogram[a->c] - colorHistogram[b->c];
    }
    return b->size - a->size;
}

int byColorDescAndFromLargest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return colorHistogram[b->c] - colorHistogram[a->c];
    }
    return b->size - a->size;
}

int byColorAndFromTop(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return colorHistogram[a->c] - colorHistogram[b->c];
    }
    return b->y - a->y;
}

int byColorAndFromTop2(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return colorHistogram[a->c] - colorHistogram[b->c];
    }
    return b->minY - a->minY;
}

int byColorAndFromTop3(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return colorHistogram[a->c] - colorHistogram[b->c];
    }
    return b->maxY - a->maxY;
}

int byColorNoAndFromSmallest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return a->c - b->c;
    }
    return a->size - b->size;
}

int byColorNoAndFromLargest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return a->c - b->c;
    }
    return b->size - a->size;
}

int byColorNoAndFromTop(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return a->c - b->c;
    }
    return b->y - a->y;
}

int byColorNoAndFromTop2(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return a->c - b->c;
    }
    return b->minY - a->minY;
}

int byColorNoAndFromTop3(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    if (a->c != b->c) {
        return a->c - b->c;
    }
    return b->maxY - a->maxY;
}

#ifdef USE_RAND
int randomStrategy(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    return a->rand - b->rand;
}
#endif

#ifdef VALIDATION
bool validate(Board *board, ShapeList& list) {
    int total = 0;
    bool ok = true;
    for (int i = 0; i < list.size; i++) {
        total += list.shapes[i].size;
        int x = list.shapes[i].minX;
        int y = list.shapes[i].y;
        if (board->board[x][y] != list.shapes[i].c || !list.shapes[i].c) {
            cout << "B " << char('A' + board->board[x][y]) << char('A' + list.shapes[i].c) << endl;
            list.shapes[i].print();
            ok = false;
        }
        if (list.shapes[i].y < list.shapes[i].minY || list.shapes[i].y > list.shapes[i].maxY) {
            cout << "D " << endl;
            list.shapes[i].print();
            ok = false;
        }
        if (list.shapes[i].size < 2) {
            cout << "S" << endl;
            ok = false;
        }
        if (list.shapes[i].minX < 1 || list.shapes[i].minX > list.shapes[i].maxX ||
            list.shapes[i].minY < 1 || list.shapes[i].minY > list.shapes[i].maxY ||
            list.shapes[i].y < list.shapes[i].minY || list.shapes[i].y > list.shapes[i].maxY ||
            list.shapes[i].maxX > board->w || list.shapes[i].maxY > board->h
        ) {
            cout << "T" << endl;
            ok = false;
        }
        int sum =
            (board->board[x - 1][y] == list.shapes[i].c) +
            (board->board[x + 1][y] == list.shapes[i].c) +
            (board->board[x][y - 1] == list.shapes[i].c) +
            (board->board[x][y + 1] == list.shapes[i].c);
        if (!sum) {
            cout << "C" << endl;
            list.shapes[i].print();
            board->print();
            ok = false;
        }
    }
    int total2 = 0;
    for (int x = 1; x <= board->w; x++) {
        for (int y = 1; y <= board->h; y++) {
            int sum =
                (board->board[x - 1][y] == board->board[x][y]) +
                (board->board[x + 1][y] == board->board[x][y]) +
                (board->board[x][y - 1] == board->board[x][y]) +
                (board->board[x][y + 1] == board->board[x][y]);
            if (!sum) {
                total++;
            }
            if (board->board[x][y]) {
                total2++;
            }
        }
    }
    if (total != total2) {
        cout << "A " << total << " " << total2 << endl;
        board->print();
        ok = false;
    }
    return ok;
}
#endif

void calcHistogram(Board *board) {
    memset(colorHistogram, 0, sizeof(colorHistogram));

    const int w = board->w;
    const int h = board->h;

    for (int x = 1; x <= w; x++) {
        char* col = board->board[x] + 1;
        for (int y = 1; y <= h; y++) {
            colorHistogram[*col]++;
            col++;
        }
    }

    mostPopularColor = -1;
    int mostPopularCount = 0;
    for (int c = 1; c <= MAX_COLOR; c++) {
        if (colorHistogram[c] > mostPopularCount) {
            mostPopularColor = c;
            mostPopularCount = colorHistogram[c];
        }
    }
}

void test(Board *board, int (*comparator)(const Shape*, const Shape*), Game &game) {
    ShapeList list;
    list.update(board);
    #ifdef VALIDATION
    validate(board, list);
    #endif

    game.reset();
    while(list.size) {
        Shape& move = list.findBest(comparator);
        game.move(move);
        board->remove(move);
        list.update(board, move.minX - 1, move.maxX + 1, move.minY - 1);
        #ifdef VALIDATION
        if (move.score() > 6250000) {
            move.print();
        }
        validate(board, list);
        #endif
    }
}

/*const int NCOMP = 12;
int (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromSmallest, fromLargest, fromBottom, fromTop, fromLeft, fromSmallestWithoutOne, byColorAndFromSmallest,
    byColorAndFromLargest, byColorAndFromTop, byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop
};*/
const int NCOMP_ = 25;
int (*comparators_[NCOMP_])(const Shape*, const Shape*) = {
    /*fromSmallest, */fromLargest, /*fromBottom, */fromTop, /*fromBottom2, */fromTop2, /*fromBottom3, */fromTop3, /*fromLeft,*/
    fromSmallestWithoutOne<1>, fromSmallestWithoutOne<2>, fromSmallestWithoutOne<3>, fromSmallestWithoutOne<4>,
    fromTopWithoutOne<1>, fromTopWithoutOne<2>, fromTopWithoutOne<3>, fromTopWithoutOne<4>,
    fromLargestWithoutMostPop,
    fromSmallestWithoutMostPop, byColorAndFromSmallest,
    byColorAndFromLargest, byColorDescAndFromLargest, byColorAndFromTop, byColorAndFromTop2, byColorAndFromTop3,
    byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop, byColorNoAndFromTop2, byColorNoAndFromTop3
};
const int NCOMP = 32;
int (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromTopWithoutOne<1>, fromTopWithoutOne<2>, fromTopWithoutOne<3>, fromTopWithoutOne<4>,
    fromTopWithoutOne<5>, fromTopWithoutOne<6>, fromTopWithoutOne<7>, fromTopWithoutOne<8>,
    fromTopWithoutOne<9>, fromTopWithoutOne<10>, fromTopWithoutOne<11>, fromTopWithoutOne<12>,
    fromTopWithoutOne<13>, fromTopWithoutOne<14>, fromTopWithoutOne<15>, fromTopWithoutOne<16>,
    fromTopWithoutOne<17>, fromTopWithoutOne<18>, fromTopWithoutOne<19>, fromTopWithoutOne<20>,
    fromSmallestWithoutOne<1>, fromSmallestWithoutOne<2>, fromSmallestWithoutOne<3>, fromSmallestWithoutOne<4>,
    fromSmallestWithoutOne<5>, fromSmallestWithoutOne<6>, fromSmallestWithoutOne<7>, fromSmallestWithoutOne<8>,
    fromSmallestWithoutOne<9>, fromSmallestWithoutOne<10>, fromSmallestWithoutOne<11>, fromSmallestWithoutOne<12>,
    //fromSmallestWithoutOne<13>, fromSmallestWithoutOne<14>, fromSmallestWithoutOne<15>, fromSmallestWithoutOne<16>,
    //fromSmallestWithoutOne<17>, fromSmallestWithoutOne<18>, fromSmallestWithoutOne<19>, fromSmallestWithoutOne<20>
};
/*const int NCOMP = 23;
int (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromSmallest, fromLargest, fromBottom, fromTop, fromBottom2, fromTop2, fromBottom3, fromTop3, fromLeft,
    fromSmallestWithoutOne<1>, fromLargestWithoutMostPop,
    fromSmallestWithoutMostPop, byColorAndFromSmallest,
    byColorAndFromLargest, byColorDescAndFromLargest, byColorAndFromTop, byColorAndFromTop2, byColorAndFromTop3,
    byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop, byColorNoAndFromTop2, byColorNoAndFromTop3
};*/
/*const int NCOMP = 3;
int (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromTop, byColorAndFromLargest, byColorAndFromTop
};*/
//const int NCOMP2 = 10;
//int (*comparators2[NCOMP2])(const Shape*, const Shape*) = {
//    fromLargest, fromTop, /*fromSmallestWithoutOne, */fromLargestWithoutMostPop, fromSmallestWithoutMostPop, byColorAndFromSmallest,
//    byColorAndFromLargest, byColorAndFromTop, byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop
//};
const int NCOMP2 = NCOMP;
int (**comparators2)(const Shape*, const Shape*) = comparators;
/*const int NCOMP2 = 6;
int (*comparators2[NCOMP2])(const Shape*, const Shape*) = {
    fromTop, fromSmallestWithoutMostPop, byColorAndFromSmallest,
    byColorAndFromTop, byColorNoAndFromSmallest, byColorNoAndFromTop
};*/



Game games2[NCOMP];
int hist[NCOMP] = {0};
Game* test2(Board *board) {
    calcHistogram(board);
    int bestGame = 0;
    long bestScore = -1;
    for (int i = 0; i < NCOMP; i++) {
        if (colorHistogram[i % 20 + 1]) {
            Board board2 = *board;
            test(&board2, comparators[i], games2[i]);
            if (games2[i].total > bestScore) {
                bestGame = i;
                bestScore = games2[i].total;
            }
        }
    }
    #ifdef VALIDATION
    if (bestGame < 0 || bestGame >= NCOMP) {
        cout << "wrong game " << bestGame << endl;
    }
    #endif
    hist[bestGame]++;
    return games2 + bestGame;
}

int findBestGame(Game* games, ShapeList* lists, int cnt) {
    int bestGame;
    long bestScore = -1;
    for (int i = 0; i < cnt; i++) {
        long score = games[i].total + lists[i].score();
        if (score > bestScore) {
            bestGame = i;
            bestScore = score;
        }
    }
    return bestGame;
}

#ifdef USE_ELECTIONS
Game games[NCOMP2];
Board boards[NCOMP2];
ShapeList lists[NCOMP2];
int hist2[NCOMP2] = {0};
Game* test3(Board *board, int electionPeriod) {
    calcHistogram(board);
    for (int i = 0; i < NCOMP2; i++) {
        boards[i] = *board;
        games[i].reset();
    }
    lists[0].update(board);
    for (int i = 1; i < NCOMP2; i++) {
        lists[i] = lists[0];
    }

    int moveNo = 0;
    bool change = true;
    while(change) {
        change = false;
        for (int i = 0; i < NCOMP2; i++) {
            if (lists[i].size) {
                Shape& move = lists[i].findBest(comparators2[i]);
                games[i].move(move);
                boards[i].remove(move);
                lists[i].update(boards + i, move.minX - 1, move.maxX + 1, move.minY - 1);
                change = true;
            }
        }
        moveNo++;
        if (moveNo % electionPeriod == 0) {
            int bestGame = findBestGame(games, lists, NCOMP2);
            for (int i = 0; i < NCOMP2; i++) {
                if (i != bestGame) {
                    games[i] = games[bestGame];
                    boards[i] = boards[bestGame];
                    lists[i] = lists[bestGame];
                }
            }
            calcHistogram(&boards[0]);
            hist2[bestGame]++;
        }
    }

    return games + findBestGame(games, lists, NCOMP2);
}
#endif

#ifdef USE_RAND
Move pickRandomMove(Board *board) {
    Move move;
    for (int i = 0; i < board->w * board->h; i++) {
        int x = 1 + (random() % board->w);
        int h = board->size[x];
        if (h == 0) {
            continue;
        }
        int y = 1 + (random() % h);
        char c = board->board[x][y];
        if (!c) {
            board->size[x] = y - 1;
            continue;
        }
        if (board->board[x - 1][y] == c ||
            board->board[x + 1][y] == c ||
            board->board[x][y - 1] == c ||
            board->board[x][y + 1] == c) {
            move.x = x;
            move.y = y;
            return move;
        }
    }
    move.x = -1;
    return move;
}

Move pickFirstMove(Board *board) {
    Move move;
    for (int x = 1; x <= board->w; x++) {
        int h = board->size[x];
        for (int y = h; y > 0; y--) {
            char c = board->board[x][y];
            if (!c) {
                board->size[x] = y;
                continue;
            }
            if (board->board[x - 1][y] == c ||
                board->board[x + 1][y] == c ||
                board->board[x][y - 1] == c ||
                board->board[x][y + 1] == c) {
                move.x = x;
                move.y = y;
                return move;
            }
        }
    }
    move.x = -1;
    return move;
}

void randomPlayer(Board *board, Game &game) {
    game.reset();
    for (int x = 1; x <= board->w; x++) {
        board->size[x] = board->h;
    }
    while(true) {
        Move move = pickRandomMove(board);
        if (move.x < 0) {
            break;
        }
        int score = board->remove2(move);
        game.move(move, score);
    }
    while(true) {
        Move move = pickFirstMove(board);
        if (move.x < 0) {
            break;
        }
        int score = board->remove2(move);
        game.move(move, score);
    }
}

const int NRAND = 100;
static Game randomGames[NRAND];
Game* randomPlayer2(Board *board) {
    int bestGame;
    long bestScore = -1;
    for (int i = 0; i < NRAND; i++) {
        Board board2 = *board;
        randomPlayer(&board2, randomGames[i]);
        if (randomGames[i].total > bestScore) {
            bestGame = i;
            bestScore = randomGames[i].total;
        }
    }
    return randomGames + bestGame;
}
#endif

void play() {
    int t;
    cin >> t;
    for (int i = 0; i < t; i++) {
        Board board;
        board.loadFromCin();
        Game* game = test2(&board);
        game->send(board.h);
        //cout << game->total << endl;
    }
}

#ifdef USE_RAND
void randomPlay() {
    int t;
    cin >> t;
    for (int i = 0; i < t; i++) {
        Board board;
        board.loadFromCin();
        Game* game = randomPlayer2(&board);
        game->send(board.h);
        //cout << game->total << endl;
    }
}
#endif

void stats() {
    int64_t begin = clock();
    //int width = 20;
    //int height = 50;
    long total2 = 0, total3 = 0, total4 = 0;
    for (int ncols = 4; ncols <= 20; ncols += 2) {
        long total[NCOMP + 3] = {0};
        for (int i = 0; i < 1000; i++) {
            int width = (rand() % 47) + 4;
            int height = (rand() % 47) + 4;
            Board* board = Board::randomBoard(width, height, ncols);
            /*for (int c = 0; c < NCOMP; c++) {
                Board board2 = *board;
                Game game;
                test(&board2, comparators[c], game);
                total[c] += game.total;
            }*/
            Board board2 = *board;
            Game *game = test2(&board2);
            total[NCOMP] += game->total;
            if (total[NCOMP] > 6250000000L) {
                cout << "too big " << i << " " << total[NCOMP] << " " << game->total << " " << (game - games2) << endl;
            }
            total2 += game->total * ncols * ncols / width / height;

            #ifdef USE_RAND
            board2 = *board;
            game = randomPlayer2(&board2);
            total[NCOMP + 1] += game->total;
            total3 += game->total * ncols * ncols / width / height;
            #endif

            #ifdef USE_ELECTIONS
            game = test3(board, 1);
            total[NCOMP + 2] += game->total;
            total4 += game->total * ncols * ncols / width / height;
            #endif

            delete board;
        }
        /*int best = -1;
        long bestV = -1;
        for (int c = 0; c < NCOMP; c++) {
            if (total[c] > bestV) {
                bestV = total[c];
                best = c;
            }
        }
        cout << ncols << " " << best << " " << bestV << " " << bestV * ncols * ncols / width / height <<
            " " << total[NCOMP] << " " << total[NCOMP] * ncols * ncols / width / height << endl;*/
        cout << ncols << " " << total[NCOMP];
        #ifdef USE_RAND
        cout << " " << total[NCOMP + 1];
        #endif
        #ifdef USE_ELECTIONS
        cout << " " << total[NCOMP + 2];
        #endif
        cout << /*" " << total[NCOMP] * ncols * ncols / width / height <<*/ endl;
        for (int i = 0; i < NCOMP; i++) {
            cout << ncols << " " << i << " " << hist[i];
            cout << endl;
        }
        #ifdef USE_ELECTIONS
        for (int i = 0; i < NCOMP2; i++) {
            cout << ncols << " N" << i << " " << hist2[i];
            cout << endl;
        }
        #endif
    }
    cout << total2;
    #ifdef USE_RAND
    cout << " " << total3;
    #endif
    #ifdef USE_ELECTIONS
    cout << " " << total4;
    #endif
    cout << endl;

    int64_t end = clock();
    cout << double(end - begin) / CLOCKS_PER_SEC << endl;
}

void testFill() {
    int n = 0;
    while (true) {
        Board* board = Board::randomBoard(8, 8, 20);
        ShapeList list;
        list.update(board);
        //board->print();
        //list.print();
        #ifdef VALIDATION
        if (!validate(board, list)) {
            break;
        }
        #endif
        n++;
        if (n % 1000000 == 0) {
            cout << n << " " << double(clock()) / CLOCKS_PER_SEC << endl;
        }
    }
}

void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main() {
    //signal(SIGSEGV, handler);
    //signal(SIGBUS, handler);
    stats();
    //testFill();
    //play();
    //randomPlay();
    return 0;
}
//1904074   71  2261.7  2.35
//1906054 fromLeft
//1904217 fromSmallest
//1904166 fromBottom
//1904217 fromSmallestWithoutMostPop

//1919433

//1915248
//1912668

//50 1943072 1162410
//100 1943072 1704132

//top 1952129 105.261
//top2 1946700 89
//top3 1947936 67.5345
//top2 top3 1966047 82.56
//top top2 top3 1972862 97.1351

//1965105 73.1096
//1970635 71.05
