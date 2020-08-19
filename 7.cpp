#include <stdlib.h>
#include <memory.h>

#include <algorithm>
#include <vector>
#include <iostream>
#include <cstring>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "dirent.h"

#define MAX_WIDTH 52
#define MAX_HEIGHT 52
#define MAX_HEIGHT2 64
#define MAX_COLOR 20
#define MAX_GAMES 500
//#define USE_RAND
//#define USE_ELECTIONS
//#define VALIDATION

#pragma pack(1)

using namespace std;

struct Shape;
struct ShapeList;

struct Move {
    int8_t x, y;
};

struct Shape {
    int8_t minX, maxX, minY, maxY;
    int16_t size;
    int16_t area;
    char c;
    int8_t y;
    char vs;
    int8_t rand;

    int score() const {
        return size * (size - 1);
    }

    void print() const {
        cout << "minX=" << (int)minX << " maxX=" << (int)maxX << " minY=" << (int)minY << " maxY=" << (int)maxY <<
            " y=" << (int)y << " size=" << (int)size << " c=" << char('A' + c) << " score=" << score() <<
            " vs=" << (int)vs << endl;
    }
};

struct Segment {
    uint8_t x, minY, maxY1;
};

struct Color {
    char c;
    int count;
};

int colorComparator(const void* va, const void* vb) {
    return ((const Color*) vb)->count - ((const Color*) va)->count;
}


struct Board {
    int w, h;
    char board[MAX_WIDTH][MAX_HEIGHT2];
    Color colorHistogram[MAX_COLOR + 1];
    mutable uint64_t used[MAX_WIDTH];

    #ifdef USE_RAND
    mutable int size[MAX_WIDTH];
    #endif

    void operator= (const Board& src) {
        w = src.w;
        h = src.h;
        for (int x = 0; x <= w + 1; x++) {
            memcpy(board[x], src.board[x], h + 2);
        }
        //memcpy(colorHistogram, src.colorHistogram, sizeof(colorHistogram));
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

    void print() const {
        for (int y = h; y > 0; y--) {
            for (int x = 1; x <= w; x++) {
                cout << (board[x][y] ? char('A' + board[x][y]) : '-');
            }
            cout << endl;
        }
    }

    void addSegment(int x, int minY, int maxY1, bool left, bool right, char c, Segment*& segments) const {
        const char *col = board[x];
        int nminY = minY;
        if (col[nminY] == c) {
            const char* col1 = col - 1;
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
            addSegment(x, nmaxY1 + 1, maxY1, left, right, c, segments);
        }

        uint64_t mask = (uint64_t(1) << nmaxY1) - (uint64_t(1) << nminY);
        if ((used[x] & mask) != uint64_t(0)) {
            return;
        }
        used[x] |= mask;
        Segment* segm = segments++;
        segm->x = x;
        segm->minY = nminY;
        segm->maxY1 = nmaxY1;

        if (x > 1 && left) {
            addSegment(x - 1, nminY, nmaxY1, true, false, c, segments);
        }
        if (x < w && right) {
            addSegment(x + 1, nminY, nmaxY1, false, true, c, segments);
        }
        if (nmaxY1 - maxY1 >= 2) {
            if (x > 1 && !left) {
                addSegment(x - 1, maxY1 + 1, nmaxY1, true, false, c, segments);
            }
            if (x < w && !right) {
                addSegment(x + 1, maxY1 + 1, nmaxY1, false, true, c, segments);
            }
        }
        if (minY - nminY >= 2) {
            if (x > 1 && !left) {
                addSegment(x - 1, nminY, minY - 1, true, false, c, segments);
            }
            if (x < w && !right) {
                addSegment(x + 1, nminY, minY - 1, false, true, c, segments);
            }
        }
    }

    static int segmentComparator(const void *va, const void *vb) {
        const Segment* a = (const Segment*) va;
        const Segment* b = (const Segment*) vb;
        if (a->x != b->x) {
            return a->x - b->x;
        }
        return a->maxY1 - b->maxY1;
    }

    void remove(const Shape& shape) {
        memset(used + shape.minX, 0, sizeof(uint64_t) * (shape.maxX - shape.minX + 1));
        Segment segments[MAX_WIDTH * MAX_HEIGHT];
        Segment *segm = segments;
        addSegment(shape.minX, shape.y, shape.y + 1, true, true, shape.c, segm);
        qsort(segments, segm - segments, sizeof(Segment), segmentComparator);
        segm->x = (MAX_WIDTH + 1);

        Segment *s = segments;
        char *col;
        char *dest;
        bool newLine = true;
        while (s != segm) {
            if (newLine) {
                col = board[s->x];
                dest = col + s->minY;
            }
            newLine = s[1].x != s->x;
            char* src = col + s->maxY1;
            if (*src) {
                int nminY = newLine ? h + 1 : s[1].minY;
                int len = nminY - s->maxY1;
                memcpy(dest, src, len);
                dest += len;
            }
            if (newLine) {
                while(*dest) {
                    *dest = 0;
                    dest++;
                }
                //memset(dest, 0, h + 1 - (dest - col));
            }
            s++;
        }
    }

    int remove2(const Move move) {
        memset(used, 0, sizeof(uint64_t) * (w + 2));
        Segment segments[MAX_WIDTH * MAX_HEIGHT];
        Segment *segm = segments;
        addSegment(move.x, move.y, move.y + 1, true, true, board[move.x][move.y], segm);
        qsort(segments, segm - segments, sizeof(Segment), segmentComparator);
        segm->x = (MAX_WIDTH + 1);

        Segment *s = segments;
        char *col;
        char *dest;
        bool newLine = true;
        int size = 0;
        while (s != segm) {
            size += s->maxY1 - s->minY;
            if (newLine) {
                col = board[s->x];
                dest = col + s->minY;
            }
            newLine = s[1].x != s->x;
            char* src = col + s->maxY1;
            if (*src) {
                int nminY = newLine ? h + 1 : s[1].minY;
                int len = nminY - s->maxY1;
                memcpy(dest, src, len);
                dest += len;
            }
            if (newLine) {
                while(*dest) {
                    *dest = 0;
                    dest++;
                }
                //memset(dest, 0, h + 1 - (dest - col));
            }
            s++;
        }
        return size * (size - 1);
    }
};

struct ShapeList {
    int size[2][2];
    Shape shapes[2][2][MAX_WIDTH * MAX_HEIGHT];
    char tabuColor;
    mutable int g_seed;
    mutable uint64_t used[MAX_WIDTH];

    inline int fastRand() const {
      g_seed = (214013 * g_seed + 2531011);
      return (g_seed >> 16) & 0x7FFF;
    }

    ShapeList(char tabuColor):
        tabuColor(tabuColor),
        g_seed(5325353) {
        memset(size, 0, sizeof(size));
    }

    bool isEmpty() const {
        return !size[0][0] && !size[0][1] && !size[1][0] && !size[1][1];
    }

    void operator=(const ShapeList& src) {
        memcpy(size, src.size, sizeof(size));
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                memcpy(shapes[a][b], src.shapes[a][b], size[a][b] * sizeof(Shape));
            }
        }
        tabuColor = src.tabuColor;
    }

    int addSegment(const Board* board, int x, int minY, int maxY1, bool left, bool right, Shape* dest) const {
        char c = dest->c;
        const char *col = board->board[x];
        int nminY = minY;
        if (col[nminY] == c) {
            const char* col1 = col - 1;
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

    void addShape(const Board* board, int x, int y, Shape* dest) const {
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
        dest->area = int16_t(dest->maxX - dest->minX) * dest->minY;
        dest->vs = (dest->maxX == dest->minX && !board->board[dest->minX][dest->maxY + 1])/* ||
            !(dest->maxX == dest->minX &&
                board->board[dest->minX][dest->minY - 1] == board->board[dest->minX][dest->maxY + 1])*/;
        dest->rand = fastRand();
    }

    void update(const Board *board, int minX = 1, int maxX = MAX_WIDTH, int minY = 1) {
        if (minX < 1) {
            minX = 1;
        }
        const int w = board->w;
        const int h = board->h;
        if (maxX > w) {
            maxX = w;
        }
        if (minY < 1) {
            minY = 1;
        }

        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                Shape* shapes2 = shapes[a][b];
                Shape* src = shapes2;
                int size2 = size[a][b];
                for (int i = 0; i < size2; i++) {
                    if (src->maxX >= minX && src->minX <= maxX /*&& src->maxY >= minY*/) {
                        size2--;
                        *src = shapes2[size2];
                        i--;
                        continue;
                    }
                    src++;
                }
                size[a][b] = size2;
            }
        }

        Shape* dest[2][2] = {
            {
                shapes[0][0] + size[0][0],
                shapes[0][1] + size[0][1]
            },
            {
                shapes[1][0] + size[1][0],
                shapes[1][1] + size[1][1]
            }
        };
        memset(used, 0, sizeof(uint64_t) * (w + 2));
        uint64_t* usedCol = used + minX;

        for (int x = minX; x <= maxX; x++) {
            const char* pcol = &board->board[x - 1][0/*minY*/];
            const char* col = &board->board[x][1/*minY*/];
            const char* ncol = &board->board[x + 1][0/*minY*/];
            for (int y = 1/*minY*/; y <= h; y++) {
                char c = *col;
                if (!c) {
                    break;
                }
                if (!((*usedCol >> y) & 1) &&
                (pcol[y] == c || ncol[y] == c ||
                 col[-1] == c || col[1] == c))  {
                    bool isTabu = c == tabuColor;
                    Shape* dest2 = dest[0][isTabu];
                    addShape(board, x, y, dest2);
                    if (dest2->vs) {
                        *(dest[1][isTabu]++) = *dest2;
                    } else {
                        dest[0][isTabu]++;
                    }
                }
                col++;
            }
            usedCol++;
        }
        size[0][0] = dest[0][0] - shapes[0][0];
        size[0][1] = dest[0][1] - shapes[0][1];
        size[1][0] = dest[1][0] - shapes[1][0];
        size[1][1] = dest[1][1] - shapes[1][1];
    }

    int update2(const Board *board, int minX = 1, int maxX = MAX_WIDTH, int minY = 1) {
        int profit = 0;
        if (minX < 1) {
            minX = 1;
        }
        const int w = board->w;
        const int h = board->h;
        if (maxX > w) {
            maxX = w;
        }
        if (minY < 1) {
            minY = 1;
        }

        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                Shape* shapes2 = shapes[a][b];
                Shape* src = shapes2;
                int size2 = size[a][b];
                for (int i = 0; i < size2; i++) {
                    if (src->maxX >= minX && src->minX <= maxX /*&& src->maxY >= minY*/) {
                        profit -= src->score();
                        size2--;
                        *src = shapes2[size2];
                        i--;
                        continue;
                    }
                    src++;
                }
                size[a][b] = size2;
            }
        }

        Shape* dest[2][2] = {
            {
                shapes[0][0] + size[0][0],
                shapes[0][1] + size[0][1]
            },
            {
                shapes[1][0] + size[1][0],
                shapes[1][1] + size[1][1]
            }
        };
        memset(used, 0, sizeof(uint64_t) * (w + 2));
        uint64_t* usedCol = used + minX;

        for (int x = minX; x <= maxX; x++) {
            const char* pcol = &board->board[x - 1][0/*minY*/];
            const char* col = &board->board[x][1/*minY*/];
            const char* ncol = &board->board[x + 1][0/*minY*/];
            for (int y = 1/*minY*/; y <= h; y++) {
                char c = *col;
                if (!c) {
                    break;
                }
                if (!((*usedCol >> y) & 1) &&
                (pcol[y] == c || ncol[y] == c ||
                 col[-1] == c || col[1] == c))  {
                    bool isTabu = c == tabuColor;
                    Shape* dest2 = dest[0][isTabu];
                    addShape(board, x, y, dest2);
                    profit += dest2->score();
                    if (dest2->vs) {
                        *(dest[1][isTabu]++) = *dest2;
                    } else {
                        dest[0][isTabu]++;
                    }
                }
                col++;
            }
            usedCol++;
        }
        size[0][0] = dest[0][0] - shapes[0][0];
        size[0][1] = dest[0][1] - shapes[0][1];
        size[1][0] = dest[1][0] - shapes[1][0];
        size[1][1] = dest[1][1] - shapes[1][1];
        return profit;
    }

    long score() const {
        long total = 0;
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                const Shape* shapes2 = shapes[a][b];
                for (int i = size[a][b] - 1; i >= 0; i--) {
                    total += shapes2[i].score();
                }
            }
        }
        return total;
    }

    void print() const {
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                cout << "Branch " << a << b << endl;
                for (int i = 0; i < size[a][b]; i++) {
                    shapes[a][b][i].print();
                }
            }
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

    void move(const Shape &shape) {
        total += shape.score();
        #ifdef VALIDATION
        if (total > 6250000) {
            cout << "TOTAL " << total << endl;
            shape.print();
        }
        if (nmoves > MAX_WIDTH * MAX_HEIGHT) {
            cout << "MAX " << nmoves << endl;
        }
        /*if (nmoves && moves[nmoves - 1].x == shape.minX && moves[nmoves - 1].y == shape.y) {
            cout << "SAME " << endl;
            shape.print();
        }*/
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

    string getOutput(int height) const {
        string out;
        out += "Y\n";
        for (int i = 0; i < nmoves; i++) {
            out += to_string(height - moves[i].y) + ' ' + to_string(moves[i].x - 1) + '\n';
        }
        out += "-1 -1\n";
        return out;
    }
};

/*bool fromSmallest(const Shape *a, const Shape *b) {
    return a->size < b->size;
}

bool fromLargest(const Shape *a, const Shape *b) {
    return a->size > b->size;
}

bool fromTop(const Shape *a, const Shape *b) {
    return a->y > b->y;
}

bool fromBottom(const Shape *a, const Shape *b) {
    return a->y < b->y;
}

bool fromTop2(const Shape *a, const Shape *b) {
    return a->minY > b->minY;
}

bool fromBottom2(const Shape *a, const Shape *b) {
    return a->minY < b->minY;
}

bool fromTop3(const Shape *a, const Shape *b) {
    return a->maxY > b->maxY;
}

bool fromBottom3(const Shape *a, const Shape *b) {
    return a->maxY < b->maxY;
}

bool fromLeft(const Shape *a, const Shape *b) {
    return a->minX < b->minX;
}

template<int chosenOne> bool fromSmallestWithoutOne(const Shape *a, const Shape *b) {
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca < cb;
    }
    return a->size < b->size;
}

template<int chosenOne> bool fromTopWithoutOne(const Shape *a, const Shape *b) {
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca < cb;
    }
    return b->y < a->y;
}

template<int chosenOne> bool fromWidestWithoutOne(const Shape *a, const Shape *b) {
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca < cb;
    }
    int width1 = a->maxX - a->minX;
    int width2 = b->maxX - b->minX;
    if (width1 != width2) {
        return width2 < width1;
    }
    return b->y < a->y;
}

template<int chosenOne> bool byAreaWithoutOne(const Shape *a, const Shape *b) {
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca < cb;
    }
    return b->area < a->area;
}

bool byColorAndFromSmallest(const Shape *a, const Shape *b) {
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return a->size < b->size;
}

bool byColorAndFromLargest(const Shape *a, const Shape *b) {
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->size < a->size;
}

bool byColorDescAndFromLargest(const Shape *a, const Shape *b) {
    if (a->c != b->c) {
        return a->c < b->c;
    }
    return b->size < a->size;
}

bool byColorAndFromWidest(const Shape *a, const Shape *b) {
    if (a->c != b->c) {
        return b->c < a->c;
    }
    int width1 = a->maxX - a->minX;
    int width2 = b->maxX - b->minX;
    if (width1 != width2) {
        return width2 < width1;
    }
    return b->y < a->y;
}

bool byColorAndArea(const Shape *a, const Shape *b) {
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->area < a->area;
}

bool byColorAndFromTop(const Shape *a, const Shape *b) {
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->y < a->y;
}

bool byColorAndFromTop2(const Shape *a, const Shape *b) {
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->minY < a->minY;
}

bool byColorAndFromTop3(const Shape *a, const Shape *b) {
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->maxY < a->maxY;
}

#ifdef USE_RAND_STRATEGY
bool randomStrategy(const Shape *a, const Shape *b) {
    return a->rand < b->rand;
}
#endif
*/

#ifdef VALIDATION
bool validate(const Board* board, const ShapeList& list) {
    int total = 0;
    bool ok = true;
    for (int a = 0; a < 2; a++) {
        for (int b = 0; b < 2; b++) {
            const Shape* shapes = list.shapes[a][b];
            for (int i = 0; i < list.size[a][b]; i++) {
                const Shape& shape = shapes[i];
                total += shape.size;
                int x = shape.minX;
                int y = shape.y;
                if (board->board[x][y] != shape.c || !shape.c) {
                    cout << "B " << char('A' + board->board[x][y]) << char('A' + shape.c) << endl;
                    shape.print();
                    ok = false;
                }
                if (a != shape.vs) {
                    cout << "V " << a << endl;
                    shape.print();
                    ok = false;
                }
                if (b != (shape.c == list.tabuColor)) {
                    cout << "T " << b << " " << char('A' + list.tabuColor) << endl;
                    shape.print();
                    ok = false;
                }
                if (shape.y < shape.minY || shape.y > shape.maxY) {
                    cout << "D " << endl;
                    shape.print();
                    ok = false;
                }
                if (shape.size < 2) {
                    cout << "S" << endl;
                    ok = false;
                }
                if (shape.minX < 1 || shape.minX > shape.maxX ||
                    shape.minY < 1 || shape.minY > shape.maxY ||
                    shape.y < shape.minY || shape.y > shape.maxY ||
                    shape.maxX > board->w || shape.maxY > board->h
                ) {
                    cout << "T" << endl;
                    ok = false;
                }
                int sum =
                    (board->board[x - 1][y] == shape.c) +
                    (board->board[x + 1][y] == shape.c) +
                    (board->board[x][y - 1] == shape.c) +
                    (board->board[x][y + 1] == shape.c);
                if (!sum) {
                    cout << "C" << endl;
                    shape.print();
                    board->print();
                    ok = false;
                }
            }
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

class Strategy {
protected:
    char tabuColor;

public:
    Strategy(char tabuColor): tabuColor(tabuColor) {
    }

    char getTabu() const {
        return tabuColor;
    }

    virtual const Shape* findBest(ShapeList& list) = 0;

    virtual void print() const = 0;

    virtual void play(Board *board, Game &game, int seed = 0) {
        ShapeList list(tabuColor);
        if (seed) {
            list.g_seed = seed;
        }
        list.update(board);
        #ifdef VALIDATION
        validate(board, list);
        #endif
        game.reset();
        while(!list.isEmpty()) {
            const Shape* move = findBest(list);
            #ifdef VALIDATION
            if (!move) {
                list.print();
            }
            #endif
            game.move(*move);
            board->remove(*move);
            list.update(board, move->minX - 1, move->maxX + 1, move->minY - 1);
            #ifdef VALIDATION
            if (move->score() > 6250000) {
                move->print();
            }
            validate(board, list);
            #endif
        }
    }

    virtual ~Strategy() {
    }
};

class FromTop: public Strategy {
public:
    FromTop(): Strategy(-1) {
    }

    virtual const Shape* findBest(ShapeList& list) {
        for (int a = 0; a < 2; a++) {
            const Shape* best = NULL;
            int8_t y = -1;
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    while(shape != end) {
                        if (shape->y > y || (shape->y == y && shape->rand > best->rand)) {
                            best = shape;
                            y = shape->y;
                        }
                        shape++;
                    }
                }
            }
            if (best) {
                return best;
            }
        }
        return NULL;
    }

    virtual void print() const {
        cout << "FromTop()" << endl;
    }
};



class FromTopWithTabu: public Strategy {
public:
    FromTopWithTabu(char tabuColor): Strategy(tabuColor) {
    }

    virtual const Shape* findBest(ShapeList& list) {
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    const Shape* best = shape++;
                    int8_t y = best->y;
                    while(shape != end) {
                        if (shape->y > y || (shape->y == y && shape->rand > best->rand)) {
                            best = shape;
                            y = shape->y;
                        }
                        shape++;
                    }
                    return best;
                }
            }
        }
        return NULL;
    }

    virtual void print() const {
        cout << "FromTopWithTabu(" << int(tabuColor) << ")" << endl;
    }
};

class ByWidthWithTabu: public Strategy {
public:
    ByWidthWithTabu(char tabuColor): Strategy(tabuColor) {
    }

    virtual const Shape* findBest(ShapeList& list) {
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    const Shape* best = NULL;
                    int8_t w = -1;
                    while(shape != end) {
                        int8_t width = shape->maxX - shape->minX;
                        if (width > w || (width == w && (shape->y > best->y || (shape->y == best->y && shape->rand > best->rand)))) {
                            best = shape;
                            w = width;
                        }
                        shape++;
                    }
                    return best;
                }
            }
        }
        return NULL;
    }

    virtual void print() const {
        cout << "ByWidthWithTabu(" << int(tabuColor) << ")" << endl;
    }
};

class ByAreaWithTabu: public Strategy {
public:
    ByAreaWithTabu(char tabuColor): Strategy(tabuColor) {
    }

    virtual const Shape* findBest(ShapeList& list) {
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    const Shape* best = shape++;
                    int16_t area= best->area;
                    while(shape != end) {
                        if (shape->area > area || (shape->area == area && shape->rand > best->rand)) {
                            best = shape;
                            area = shape->area;
                        }
                        shape++;
                    }
                    return best;
                }
            }
        }
        return NULL;
    }

    virtual void print() const {
        cout << "ByAreaWithTabu(" << int(tabuColor) << ")" << endl;
    }
};

class ByColorAndArea: public Strategy {
public:
    ByColorAndArea(): Strategy(-1) {
    }

    virtual const Shape* findBest(ShapeList& list) {
        for (int a = 0; a < 2; a++) {
            const Shape* best = NULL;
            int bestCol = -1;
            int16_t bestArea= -1;
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    while(shape != end) {
                        char col = shape->c;
                        if (col > bestCol || (col == bestCol && (shape->area > bestArea || (shape->area == bestArea && shape->rand > best->rand)))) {
                            best = shape;
                            bestCol = col;
                            bestArea = shape->area;
                        }
                        shape++;
                    }
                }
            }
            if (best) {
                return best;
            }
        }
        return NULL;
    }

    virtual void print() const {
        cout << "ByColorAndArea()" << endl;
    }
};

class DualByAreaWithTabu: public Strategy {
public:
    DualByAreaWithTabu(char tabuColor): Strategy(tabuColor) {
    }

    virtual const Shape* findBest(ShapeList& list) {
        return NULL;
    }

    void findBest2(ShapeList& list, Shape* results) {
        const Shape* best1 = NULL;
        const Shape* best2 = NULL;
        int16_t area1 = -1;
        int16_t area2 = -1;
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
    //cout << __LINE__ << endl;
                int size = list.size[a][b];
                if (size) {
    //cout << __LINE__ << endl;
                    const Shape* shape = list.shapes[a][b];
    //cout << __LINE__ << endl;
                    const Shape* end = shape + size;
    //cout << __LINE__ << endl;
                    while(shape != end) {
    //cout << __LINE__ << endl;
                        if (shape->area > area2 || (shape->area == area2 && shape->rand > best2->rand)) {
    //cout << __LINE__ << endl;
                            if (shape->area > area1 || (shape->area == area1 && shape->rand > best1->rand)) {
    //cout << __LINE__ << endl;
                                best2 = best1;
                                area2 = area1;
                                best1 = shape;
                                area1 = shape->area;
                            } else {
    //cout << __LINE__ << endl;
                                best2 = shape;
                                area2 = shape->area;
                            }
    //cout << __LINE__ << endl;
                        }
                        shape++;
                    }
    //cout << __LINE__ << endl;
                    if (best2) {
    //cout << __LINE__ << endl;
                        results[0] = *best1;
                        results[1] = *best2;
    //cout << __LINE__ << endl;
                        return;
                    }
                }
            }
        }
    //cout << __LINE__ << endl;
        results[0] = *best1;
        results[1] = best2 ? *best2 : *best1;
    //cout << __LINE__ << endl;
    }

    virtual void play(Board *board1, Game &game, int seed = 0) {
    //cout << __LINE__ << endl;
        ShapeList list1(tabuColor);
        ShapeList list2(tabuColor);
        if (seed) {
            list1.g_seed = seed;
            list2.g_seed = seed + 2000;
        }

        Board board2 = *board1;
        list1.update(board1);
        list2.update(&board2);

    //cout << __LINE__ << endl;
        #ifdef VALIDATION
        validate(board1, list1);
        validate(&board2, list2);
        #endif

        game.reset();
        while(!list1.isEmpty()) {
    //cout << __LINE__ << endl;
            Shape moves[2];
            findBest2(list1, moves);
    //cout << __LINE__ << endl;
    //cout << "r0";
    //moves[0]->print();
            board1->remove(moves[0]);
    //cout << "ra";
    //moves[0]->print();
            int profit1 = moves[0].score() + list1.update2(board1, moves[0].minX - 1, moves[0].maxX + 1, moves[0].minY - 1);
    //cout << "rb";
    //moves[0]->print();
    //cout << "r1";
    //moves[1]->print();
            board2.remove(moves[1]);
            int profit2 = moves[1].score() + list2.update2(&board2, moves[1].minX - 1, moves[1].maxX + 1, moves[1].minY - 1);
    //cout << __LINE__ << endl;

            #ifdef VALIDATION
            if (moves[0].score() > 6250000) {
                moves[0].print();
            }
    //cout << "r2";
    //moves[0]->print();
            validate(board1, list1);
            if (moves[1].score() > 6250000) {
                moves[1].print();
            }
            validate(&board2, list2);
            #endif

    //cout << "r3";
    //moves[0]->print();
    //moves[0]->print();
    //if (moves[1])
      //  moves[1]->print();
    //cout << __LINE__ << " " << profit1 << " " << profit2 << " " << game.nmoves << endl;
            if (profit1 >= profit2) {
    //cout << __LINE__ << endl;
    //cout << "0 ";
    //moves[0].print();
    //moves[1].print();
                game.move(moves[0]);
    //cout << __LINE__ << endl;
                board2 = *board1;
    //cout << __LINE__ << endl;
                list2 = list1;
            } else {
    //cout << __LINE__ << endl;
    //cout << "1 ";
    //moves[1].print();
    //moves[0].print();
                /*#ifdef VALIDATION
                if (game.nmoves && game.moves[game.nmoves - 1].x == moves[1].minX && game.moves[game.nmoves - 1].y == moves[1].y) {
                    //board2.print();
                    //moves[1]->print();
                    exit(0);
                }
                #endif*/
                game.move(moves[1]);
    //cout << __LINE__ << endl;
                *board1 = board2;
    //cout << __LINE__ << endl;
                list1 = list2;
            }
            #ifdef VALIDATION
            validate(board1, list1);
            validate(&board2, list2);
            #endif
    //cout << __LINE__ << endl;
    //board1->print();
        }
    }

    virtual void print() const {
        cout << "DualByAreaWithTabu(" << int(tabuColor) << ")" << endl;
    }
};

void calcHistogram(Board *board) {
    memset(board -> colorHistogram, 0, sizeof(board -> colorHistogram));

    const int w = board->w;
    const int h = board->h;

    for (int c = 1; c <= MAX_COLOR; c++) {
        board->colorHistogram[c].c = c;
    }

    for (int x = 1; x <= w; x++) {
        const char* col = board->board[x] + 1;
        for (int y = 1; y <= h; y++) {
            if (*col) {
                board->colorHistogram[*col].count++;
            }
            col++;
        }
    }

    qsort(board->colorHistogram + 1, MAX_COLOR, sizeof(Color), colorComparator);
    int map[MAX_COLOR + 1];
    for (int c = 1; c <= MAX_COLOR; c++) {
        map[board->colorHistogram[c].c] = c;
    }

    for (int x = 1; x <= w; x++) {
        char* col = board->board[x];
        for (int y = 1; y <= h; y++) {
            col[y] = map[col[y]];
        }
    }
}


///*const int NCOMP = 9;
//bool (*comparators[NCOMP])(const Shape*, const Shape*) = {
//    fromSmallest, fromLargest, fromBottom, fromTop, fromLeft, fromSmallestWithoutOne, byColorAndFromSmallest,
//    byColorAndFromLargest, byColorAndFromTop
//};*/
//const int NCOMP_ = 18;
//bool (*comparators_[NCOMP_])(const Shape*, const Shape*) = {
//    /*fromSmallest, */fromLargest, /*fromBottom, */fromTop, /*fromBottom2, */fromTop2, /*fromBottom3, */fromTop3, /*fromLeft,*/
//    fromSmallestWithoutOne<1>, fromSmallestWithoutOne<2>, fromSmallestWithoutOne<3>, fromSmallestWithoutOne<4>,
//    fromTopWithoutOne<1>, fromTopWithoutOne<2>, fromTopWithoutOne<3>, fromTopWithoutOne<4>,
//    byColorAndFromSmallest,
//    byColorAndFromLargest, byColorDescAndFromLargest, byColorAndFromTop, byColorAndFromTop2, byColorAndFromTop3
//};
//const int NCOMP = 29;
//bool (*comparators[NCOMP])(const Shape*, const Shape*) = {
//    byAreaWithoutOne<1>, byAreaWithoutOne<2>, byAreaWithoutOne<3>, byAreaWithoutOne<4>,
//    byAreaWithoutOne<5>, byAreaWithoutOne<6>, byAreaWithoutOne<7>, byAreaWithoutOne<8>,
//    byAreaWithoutOne<9>, byAreaWithoutOne<10>, byAreaWithoutOne<11>, byAreaWithoutOne<12>,
//    byAreaWithoutOne<13>, byAreaWithoutOne<14>, byAreaWithoutOne<15>, byAreaWithoutOne<16>,
//    byAreaWithoutOne<17>, byAreaWithoutOne<18>, byAreaWithoutOne<19>, byAreaWithoutOne<20>,
//    fromTopWithoutOne<1>, fromTopWithoutOne<2>, fromTopWithoutOne<3>, fromTopWithoutOne<4>,
//    //fromTopWithoutOne<5>, //fromTopWithoutOne<6>,
//    fromWidestWithoutOne<1>, fromWidestWithoutOne<2>, fromWidestWithoutOne<3>, fromWidestWithoutOne<4>,
//    //fromWidestWithoutOne<5>, //fromWidestWithoutOne<6>, fromTopWithoutOne<7>, fromTopWithoutOne<8>,
//    //fromTopWithoutOne<9>, fromTopWithoutOne<10>, //fromTopWithoutOne<11>, fromTopWithoutOne<12>,
//    //fromTopWithoutOne<13>, fromTopWithoutOne<14>, //fromTopWithoutOne<15>, fromTopWithoutOne<16>,
//    //fromTopWithoutOne<17>, fromTopWithoutOne<18>, fromTopWithoutOne<19>, fromTopWithoutOne<20>
//    /*byColorAndFromWidest,*/ byColorAndArea
//};
///*const int NCOMP = 23;
//bool (*comparators[NCOMP])(const Shape*, const Shape*) = {
//    fromSmallest, fromLargest, fromBottom, fromTop, fromBottom2, fromTop2, fromBottom3, fromTop3, fromLeft,
//    fromSmallestWithoutOne<1>, fromLargestWithoutMostPop,
//    fromSmallestWithoutMostPop, byColorAndFromSmallest,
//    byColorAndFromLargest, byColorDescAndFromLargest, byColorAndFromTop, byColorAndFromTop2, byColorAndFromTop3,
//    byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop, byColorNoAndFromTop2, byColorNoAndFromTop3
//};*/
///*const int NCOMP = 3;
//bool (*comparators[NCOMP])(const Shape*, const Shape*) = {
//    fromTop, byColorAndFromLargest, byColorAndFromTop
//};*/
////const int NCOMP2 = 10;
////bool (*comparators2[NCOMP2])(const Shape*, const Shape*) = {
////    fromLargest, fromTop, /*fromSmallestWithoutOne, */fromLargestWithoutMostPop, fromSmallestWithoutMostPop, byColorAndFromSmallest,
////    byColorAndFromLargest, byColorAndFromTop, byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop
////};
//const int NCOMP2 = NCOMP;
//bool (**comparators2)(const Shape*, const Shape*) = comparators;
///*const int NCOMP2 = 6;
//bool (*comparators2[NCOMP2])(const Shape*, const Shape*) = {
//    fromTop, fromSmallestWithoutMostPop, byColorAndFromSmallest,
//    byColorAndFromTop, byColorNoAndFromSmallest, byColorNoAndFromTop
//};*/

const int NCOMP = 1000;

Game* compare(Board *board, vector<Strategy*>& strategies, Game* games) {
    int bestGame = 0;
    long bestScore = -1;
    for (int i = 0; i < strategies.size(); i++) {
        Board board2 = *board;
        strategies[i]->play(&board2, games[i], 1000 + i);
        if (games[i].total > bestScore) {
            bestGame = i;
            bestScore = games[i].total;
        }
    }
    #ifdef VALIDATION
    if (bestGame < 0 || bestGame >= strategies.size()) {
        cout << "wrong game " << bestGame << endl;
    }
    #endif
    return games + bestGame;
}

Game* compare(Board *board) {
    calcHistogram(board);

    vector<Strategy*> strategies;
    /*for (int k = 0; k < 10; k++) {
        for (int c = 0; !c || (colorHistogram[c].count && c <= MAX_COLOR); c++) {
            strategies.push_back(new FromTopWithTabu(c));
            strategies.push_back(new ByWidthWithTabu(c));
            strategies.push_back(new ByAreaWithTabu(c));
        }
        strategies.push_back(new ByColorAndArea());
        strategies.push_back(new FromTop());
    }*/

    strategies.push_back(new DualByAreaWithTabu(1));
    strategies.push_back(new DualByAreaWithTabu(2));
    if (!board->colorHistogram[11].count) {
        strategies.push_back(new DualByAreaWithTabu(3));
        strategies.push_back(new DualByAreaWithTabu(4));
        strategies.push_back(new DualByAreaWithTabu(5));
        if (board->w < 25 /*|| !board->colorHistogram[9].count*/) {
            strategies.push_back(new ByAreaWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(2));
            strategies.push_back(new ByWidthWithTabu(1));
            strategies.push_back(new FromTopWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(3));
            strategies.push_back(new ByColorAndArea());
            strategies.push_back(new ByWidthWithTabu(2));
            strategies.push_back(new FromTopWithTabu(2));
            strategies.push_back(new ByAreaWithTabu(1));
            strategies.push_back(new FromTopWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(4));
            strategies.push_back(new FromTopWithTabu(3));
            strategies.push_back(new ByAreaWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(2));
            //strategies.push_back(new ByWidthWithTabu(3));
            //strategies.push_back(new ByWidthWithTabu(4));
            //strategies.push_back(new FromTopWithTabu(2));
            //strategies.push_back(new FromTopWithTabu(4));
            /*strategies.push_back(new FromTopWithTabu(5));

            strategies.push_back(new ByAreaWithTabu(3));
            strategies.push_back(new FromTopWithTabu(1));
            strategies.push_back(new ByWidthWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(1));
            strategies.push_back(new ByWidthWithTabu(3));
            strategies.push_back(new ByAreaWithTabu(5));
            strategies.push_back(new ByAreaWithTabu(2));
            strategies.push_back(new ByWidthWithTabu(3));*/
        } else {
            strategies.push_back(new ByAreaWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(2));
            strategies.push_back(new FromTopWithTabu(1));
            strategies.push_back(new ByWidthWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(3));
            strategies.push_back(new FromTopWithTabu(2));
            strategies.push_back(new ByWidthWithTabu(2));
            strategies.push_back(new ByAreaWithTabu(1));
            strategies.push_back(new FromTopWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(1));
            strategies.push_back(new ByColorAndArea());
            strategies.push_back(new ByWidthWithTabu(3));
            strategies.push_back(new ByAreaWithTabu(3));
            strategies.push_back(new ByAreaWithTabu(2));
            //strategies.push_back(new FromTopWithTabu(1));
            //strategies.push_back(new ByWidthWithTabu(1));
            //strategies.push_back(new ByAreaWithTabu(4));
            //strategies.push_back(new ByAreaWithTabu(1));
            /*strategies.push_back(new ByWidthWithTabu(3));
            strategies.push_back(new ByAreaWithTabu(5));
            strategies.push_back(new ByAreaWithTabu(2));
            strategies.push_back(new FromTopWithTabu(3));
            strategies.push_back(new ByWidthWithTabu(3));*/
        }
        //strategies.push_back(new ByAreaWithTabu(1));
        /*for (int c = 5; board->colorHistogram[c].count && c <= 6; c++) {
            strategies.push_back(new ByAreaWithTabu(c));
            strategies.push_back(new FromTopWithTabu(c));
        }*/
        if (!board->colorHistogram[7].count) {
            strategies.push_back(new ByAreaWithTabu(1));
            strategies.push_back(new ByAreaWithTabu(2));
        }
    } else {
        /*strategies.push_back(new ByAreaWithTabu(1));
        strategies.push_back(new ByAreaWithTabu(2));
        strategies.push_back(new ByWidthWithTabu(1));
        strategies.push_back(new FromTopWithTabu(1));
        strategies.push_back(new ByAreaWithTabu(3));
        strategies.push_back(new ByColorAndArea());
        strategies.push_back(new ByWidthWithTabu(2));
        for (int c = 1; board->colorHistogram[c].count && c <= 12; c++) {
            strategies.push_back(new ByAreaWithTabu(c));
        }*/

        /*for (int c = 1; board->colorHistogram[c].count && c <= 12; c++) {
            strategies.push_back(new ByAreaWithTabu(c));
        }
        for (int c = 1; board->colorHistogram[c].count && c <= 2; c++) {
            strategies.push_back(new FromTopWithTabu(c));
        }
        for (int c = 1; board->colorHistogram[c].count && c <= 2; c++) {
            strategies.push_back(new ByWidthWithTabu(c));
        }
        strategies.push_back(new ByAreaWithTabu(1));
        for (int c = 1; board->colorHistogram[c].count && c <= 2; c++) {
            strategies.push_back(new ByAreaWithTabu(c));
        }*/

        for (int c = 1; board->colorHistogram[c].count && c <= MAX_COLOR; c++) {
            strategies.push_back(new ByAreaWithTabu(c));
        }
        for (int c = 1; board->colorHistogram[c].count && c <= 2; c++) {
            strategies.push_back(new FromTopWithTabu(c));
        }
        for (int c = 1; board->colorHistogram[c].count && c <= 2; c++) {
            strategies.push_back(new ByWidthWithTabu(c));
        }
        strategies.push_back(new ByAreaWithTabu(1));
        /*for (int c = 1; board->colorHistogram[c].count && c <= 2; c++) {
            strategies.push_back(new ByAreaWithTabu(c));
        }
        strategies.push_back(new ByColorAndArea());*/
    }

    Game games[strategies.size()];
    Game* best = compare(board, strategies, games);

    for (int i = 0; i < strategies.size(); i++) {
        delete strategies[i];
    }
    return best;
}

int findBestGame(const Game* games, const ShapeList* lists, int cnt) {
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
            if (!lists[i].isEmpty) {
                const Shape& move = lists[i].findBest(comparators2[i]);
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
        }
    }

    return games + findBestGame(games, lists, NCOMP2);
}
#endif

#ifdef USE_RAND
Move pickRandomMove(const Board *board) {
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

Move pickFirstMove(const Board *board) {
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

const int NRAND = 1000;
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

void solve(Board &board, string& out) {
    Game* game = compare(&board);
    out = game ? game->getOutput(board.h) : "N\n";
}

void play() {
    int t;
    cin >> t;

    static Board boards[MAX_GAMES];
    for (int i = 0; i < t; i++) {
        boards[i].loadFromCin();
    }

    static string out[MAX_GAMES];
    for (int i = 0; i < t; i++) {
        solve(boards[i], out[i]);
    }

    for (int i = 0; i < t; i++) {
        cout << out[i];
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
        cout << game->getOutput(board.h);
    }
}
#endif

void stats() {
    int64_t begin = clock();
    //int width = 20;
    //int height = 50;
    long total2 = 0, total3 = 0, total4 = 0;
    for (int ncols = 5; ncols <= 19; ncols ++) {
        long total[NCOMP + 3] = {0};
        for (int i = 0; i < 1000; i++) {
            int width = (rand() % 4) * 10 + 20;//(rand() % 47) + 4;
            int height = width; //(rand() % 47) + 4;
            Board* board = Board::randomBoard(width, height, ncols);
            Board board2 = *board;
            Game *game = compare(&board2);
            long score = game->total * ncols * ncols / width / height;
            total[NCOMP] += score;
            if (total[NCOMP] > 6250000000L) {
                cout << "too big " << i << " " << total[NCOMP] << " " << game->total << endl;
            }
            total2 += score;

            #ifdef USE_RAND
            board2 = *board;
            game = randomPlayer2(&board2);
            total[NCOMP + 1] += game->total;
            total3 += game->total * ncols * ncols / width / height;
            #endif

            #ifdef USE_ELECTIONS
            game = test3(board, 5);
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
        Board* board = Board::randomBoard(8, 8, 3);
        ShapeList list(3);
        list.update(board);
        board->print();
        list.print();
        break;
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

struct Combo {
    string strategy;
    int64_t time;
    long result;
};

bool comboComparator(const Combo& a, const Combo& b) {
    int64_t diff = a.time - b.time;
    if (diff < 0) {
        return true;
    }
    if (diff > 0) {
        return false;
    }
    return a.result > b.result;
}

void optimalSet(int ncols) {
    static vector<Combo> results;
    Game games[200];

    for (int a0 = 0; a0 <= 20 && a0 <= ncols; a0++) {
    for (int t0 = 0; t0 <= 10 && t0 <= ncols; t0++) {
    for (int w0 = 0; w0 <= 10 && w0 <= ncols; w0++) {
    for (int a1 = 0; a1 <= 10 && a1 <= a0; a1++) {
    for (int t1 = 0; t1 <= 10 && t1 <= t0; t1++) {
    for (int bca = 0; bca <= 1; bca++) {
        vector<Strategy*> strategies;
        for (int c = 1; c <= a0; c++) {
            strategies.push_back(new ByAreaWithTabu(c));
        }
        for (int c = 1; c <= t0; c++) {
            strategies.push_back(new FromTopWithTabu(c));
        }
        for (int c = 1; c <= w0; c++) {
            strategies.push_back(new ByWidthWithTabu(c));
        }
        for (int c = 1; c <= a1; c++) {
            strategies.push_back(new ByAreaWithTabu(c));
        }
        for (int c = 1; c <= t1; c++) {
            strategies.push_back(new FromTopWithTabu(c));
        }
        /*for (int c = 1; c <= w1; c++) {
            strategies.push_back(new ByWidthWithTabu(c));
        }*/
        if (bca) {
            strategies.push_back(new ByColorAndArea());
        }

        long total = 0;
        srand(1);
        int64_t begin = clock();
        for (int i = 0; i < 10000; i++) {
            int width = 5;//(rand() % 47) + 4;
            int height = 5;//(rand() % 47) + 4;
            Board* board = Board::randomBoard(width, height, ncols);
            calcHistogram(board);
            Game *game = compare(board, strategies, games);
            total += game->total * ncols * ncols / width / height;
            delete board;
        }
        int64_t end = clock();
        int64_t time = end - begin;

        Combo combo;
        char name[200];
        sprintf(name, "A%d T%d W%d A%d T%d BCA%d", a0, t0, w0, a1, t1, bca);
        combo.strategy = name;
        combo.time = time;
        combo.result = total;
        results.push_back(combo);
    }}}}
    cout << a0 << ' ' <<t0 << endl;
    }}

    sort(results.begin(), results.end(), comboComparator);

    long best = 0;
    for (int i = 0; i < results.size(); i++) {
        Combo &combo = results[i];
        if (combo.result > best) {
            best = combo.result;
            cout << combo.strategy << '\t' << double(combo.time) / CLOCKS_PER_SEC << '\t' << combo.result << endl;
        }
    }
}

void optimalSet2(int ncols, int width) {
    vector<Strategy*> strategies;
    Game games[200];

    for (int n = 0; n < 60; n++) {
        Strategy *best = NULL;
        double bestScore = 0;
        long bestTotal = 0;
        uint64_t bestTime = 0;
        for (int k = 0; k < 3 * ncols + 2; k++) {
            Strategy * strategy;
            if (k < ncols) {
                strategy = new ByAreaWithTabu(k + 1);
            } else if (k < ncols + ncols) {
                strategy = new FromTopWithTabu(k - ncols + 1);
            } else if (k < ncols * 3) {
                strategy = new ByWidthWithTabu(k - ncols * 2 + 1);
            } else if (k == ncols * 3) {
                strategy = new ByColorAndArea();
            } else {
                strategy = new FromTop();
            }

            if (k == 0) {
                strategies.push_back(strategy);
            } else {
                strategies[n] = strategy;
            }

            long total = 0;
            //srand(2);
            int64_t begin = clock();
            for (int i = 0; i < 10000; i++) {
                //int width = 20;//(random() % 47) + 4;
                int height = width;//(random() % 47) + 4;
                Board* board = Board::randomBoard(width, height, ncols);
                calcHistogram(board);
                Game *game = compare(board, strategies, games);
                total += game->total * ncols * ncols / width / height;
                delete board;
            }
            int64_t end = clock();
            int64_t time = end - begin;

            double score = total; //double(total) / time;
            if (score > bestScore) {
                if (best)
                    delete best;
                best = strategy;
                bestScore = score;
                bestTotal = total;
                bestTime = time;
            } else {
                delete strategy;
            }
        }
        strategies[n] = best;
        cout << bestScore << "\t" << bestTotal << "\t" << double(bestTime) / CLOCKS_PER_SEC << "\t";
        best->print();
    }
}

void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 3);

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
    //optimalSet2(5, 30);
    //play();
    //randomPlay();
    return 0;
}

//2199750 86.133  - 2865.06 2.89
//2208077 83.5947 - 2871.72 2.8
//2210546 81.4817 - 2870    2.71
//2211887 87.5575 - 2870.91 2.98
//2211126 83.0568 - 2870.73 2.81
//2208699 84.8509 - 2870.37 2.86

//1520976 77.6333 - 2871.72 2.8
//1522322 83.3632 - 2874.6  2.99
//1523701 80.6933 -         3.01
//1522668 73.6397 - 2876.13 2.76
//1543860 73.5066 - 2943.45 2.81
//1555233 76.4118 - 2941.29 2.93
//1547191 73.6971 - 2946.15 2.81
//1563270 74.7462 - 2973.42 2.84    12A 2S? 2W 6T BCW
//1566196 75.3075 - 2973.87 2.82    12A 4T 4W BCW
//1545330 74.1933 - 2942.37 2.89    same but area calculated strictly
//1566196 72.5724 area buffering
//1567595 75.5067 - 2974.05 2.85    12A 4T 4W BCW BCA
//1568364 76.0423 - 2973.87 2.87    12A 4T 4W BCW BCS
//1568681 78.4889 -         3.01    14A 4T 4W BCW BCA
//1568200 76.902  - 2974.95 2.89    13A 4T 4W BCW BCA
//1570113 77.8084 - 2978.19 2.92    13A 4T 5W BCA
//1559884 76.602                    13A 1T 8W BCA
//1574308 85.5233                   13A 6T 5W BCA
//1570349 80.1076                   16A 6T 3W BCA
//1565995 72.6904 - 2978.1  2.8     16A 4T 3W BCA
//1566722 74.6411 - 2979    2.9     20A 4T 3W BCA
//1564835 74.4453 - 2977.02 2.86    20A 5T 2W BCA
//1566720 75.2931 - 2972.97 2.9     20A 5T 3W
//1557370 73.3348 - 2963.88 2.84    20A 1T 1T BCW BCA
//1567057 77.0105 - 2974.5  2.94    20A 5T 3W BCA  - broken
//1569641 78.1771           3.01    20A 5T 3W BCA
//1569330 79.3966                   20A 4T 4W BCA
//1564835 74.5966 - 2977.02 2.84    20A 5T 2W BCA
//1569324 80.27                     20A 5T 4W
//1566421 75.0102           3.01    20A 4T 4W
//1565517 74.5539 - 2970.63 2.8     20A 3T 4W BCA
//1569330 78.3764 - 2979.27 2.95    20A 4T 4W BCA
//1568522 79.2541 - 2971.8  2.95    20A 3T 5W BCA
//1569330 74.7328 optimized findBest 20A 4T 4W BCA
//1572217 78.5357           3.01    20A 5T 4W BCA
//1572226 88.8596                   20A 4T 5W BCA

//1567669 66.0064 - 2955.24 2.43    20A 4T 4W BCA

//1574054 77.8696           3.01    20A 7T 5W BCA
//1573566 75.6725 - 2963.52 2.79    20A 6T 5W BCA

//after fix:
//1572524 66.3385 - 2964.87 2.47    20A 4T 4W BCA nminY
//1572768 66.0556 - 2956.59 2.37    20A 4T 4W BCA nmaxY-1

//randomized enabled:
//1565342 67.542
//1581735 68.7001 - 3005.28 2.65    20A 4T 4W BCA
//1591242 68.6877 - 3019.5  2.7     20A 4T 4W 1A
//1598565 76.2021 -         3.01    20A 4T 4W 3A
//1593290 73.1806 - 3044.25 2.76    20A 4T 4W 2A
//1588646 75.1089 - 3021.21 2.92    20A 4T 4W 2A BCA !!
//fastrand:
//1591778 71.1974 - 3030.21 2.64    20A 4T 4W 2A
//1594689 74.5441 - 3032.1  2.77    20A 4T 4W 2A BCA
//1603685 78.3453 - 3048.39 2.86    20A 4T 4W 1A 2A BCA
//1604956 78.88   - 3067.29 2.83    20A 4T 4W 1A 3A
//1607247 81.8219 - 3068.82 2.98    20A 4T 4W 1A 4A
//1607927 81.6804 - 3067.92 2.9     20A 4T 4W 1A 3A BCA
//1606786 80.7545 - 3074.76 2.99    20A 5T 4W 1A 3A

//1556701 72.3112 - 3014.91 2.73    cutoff <14
//                          3.01    cutoff <15
//                          3.01    cutoff <16

//4952792 207.036 - 3077.46 2.95
//                  3077.55 2.84
//                  3078.45 2.86
//                  3080.61 2.9

/*high limit:
5 1302321
6 994560
7 556912
8 319571
9 228015
10 190504
11 179677
12 176846
13 180302
14 184765
15 190376
16 198312
17 207421
18 217457
19 221386
5348425
3540.73

reference:
5 1271760
6 896570
7 463913
8 272431
9 205051
10 175330
11 170513
12 169744
13 174372
14 179826
15 185905
16 193286
17 202397
18 211890
19 215932
4988920
214.203
*/

/*
5 1275987
6 926045
7 475476
8 277321
9 207669
10 179440
11 171847
12 170376
13 175398
14 180448
15 186543
16 194025
17 202833
18 212518
19 216621
5052547
203.654
*/

//4962332 216.918 3080.61 2.9
//4988920 214.203 3110.49 2.82
//5011787 203.617 3096 ?
//                3110.31 2.94

/* 5 col 20x20
2.11558e+06	2115583	2.18792	ByAreaWithTabu(1)
2.37815e+06	2378149	4.31053	ByAreaWithTabu(2)
2.53524e+06	2535244	6.50174	ByWidthWithTabu(1)
2.66798e+06	2667979	10.3199	FromTopWithTabu(1)
2.72765e+06	2727651	10.7975	ByAreaWithTabu(3)
2.80499e+06	2804991	15.1934	ByColorAndArea()
2.85714e+06	2857135	17.7728	ByWidthWithTabu(2)
2.90252e+06	2902524	17.0623	FromTopWithTabu(2)
2.92925e+06	2929247	20.3663	ByAreaWithTabu(1)
2.9609e+06	2960899	21.9759	FromTopWithTabu(1)
2.97331e+06	2973309	24.6953	ByAreaWithTabu(4)
3.00003e+06	3000027	29.1186	FromTopWithTabu(3)
3.01558e+06	3015577	30.1681	ByAreaWithTabu(1)
3.03876e+06	3038755	40.9162	ByAreaWithTabu(2)
3.05226e+06	3052264	32.5322	ByWidthWithTabu(3)
3.06975e+06	3069752	42.2282	ByWidthWithTabu(4)
3.07211e+06	3072109	82.1907	FromTopWithTabu(2)
3.08254e+06	3082542	71.184	FromTopWithTabu(4)
3.08165e+06	3081650	53.6251	FromTopWithTabu(5)
*/

/* 5 col 30x30
5.33194e+06	5331936	4.32674	ByAreaWithTabu(1)
6.03129e+06	6031292	8.39085	ByAreaWithTabu(2)
6.48229e+06	6482293	13.0752	FromTopWithTabu(1)
6.77502e+06	6775024	18.9705	ByWidthWithTabu(1)
6.94526e+06	6945258	67.0854	ByAreaWithTabu(3)
7.0962e+06	7096202	28.3387	FromTopWithTabu(2)
7.18872e+06	7188716	39.3622	ByWidthWithTabu(2)
7.28362e+06	7283619	41.172	ByAreaWithTabu(1)
7.38027e+06	7380272	51.8362	FromTopWithTabu(1)
7.45005e+06	7450050	51.2256	ByAreaWithTabu(1)
7.50410e+06	7504105	57.0314	ByColorAndArea()
7.56256e+06	7562560	144.215	ByWidthWithTabu(3)
7.59942e+06	7599418	131.902	ByAreaWithTabu(3)
7.63949e+06	7639493	118	ByAreaWithTabu(2)
7.6798e+06	7679804	106.063	FromTopWithTabu(1)
7.6949e+06	7694898	76.5953	ByWidthWithTabu(1)
7.75562e+06	7755618	121.042	ByAreaWithTabu(4)
7.75879e+06	7758790	195.825	ByAreaWithTabu(1)
7.79343e+06	7793426	193.337	ByWidthWithTabu(3)
7.81e+06	7809997	465.105	ByAreaWithTabu(5)
7.81668e+06	7816675	174.106	ByAreaWithTabu(2)
7.84148e+06	7841477	175.715	FromTopWithTabu(3)
7.84931e+06	7849310	168.851	ByWidthWithTabu(3)
*/

/*20 col 20z20
1.62313e+06	1623134	1.81777	ByAreaWithTabu(20)
1.65585e+06	1655848	3.54717	ByAreaWithTabu(1)
1.67812e+06	1678120	6.90632	ByAreaWithTabu(14)
1.69366e+06	1693662	1.61633	FromTopWithTabu(17)
1.70141e+06	1701412	1.63509	ByAreaWithTabu(13)
1.70857e+06	1708570	1.91516	ByAreaWithTabu(3)
1.71846e+06	1718456	2.81886	ByAreaWithTabu(2)
1.72697e+06	1726974	2.9965	ByWidthWithTabu(1)
1.72348e+06	1723476	2.93528	ByAreaWithTabu(6)
1.72949e+06	1729494	3.16286	ByAreaWithTabu(8)
1.73435e+06	1734352	3.83998	FromTopWithTabu(16)
1.73726e+06	1737256	3.84276	ByAreaWithTabu(10)
1.74162e+06	1741620	4.57392	ByAreaWithTabu(5)
1.74507e+06	1745066	4.45521	ByAreaWithTabu(10)
1.7468e+06	1746802	4.81682	ByAreaWithTabu(1)
1.74883e+06	1748834	5.4011	ByAreaWithTabu(17)
*/
//3054.96 2.96 - bez podziały potem <= 11
//3052.17 2.91 - podział z || potem <= 11
//3052.98 2.93 - <= 12

