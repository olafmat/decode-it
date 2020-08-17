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
//#define USE_RAND
//#define USE_ELECTIONS
//#define VALIDATION

#pragma pack(1)

using namespace std;

int g_seed = 5325353;
inline int fastRand() {
  g_seed = (214013 * g_seed + 2531011);
  return (g_seed >> 16) & 0x7FFF;
}

struct Shape;
struct ShapeList;

uint64_t used[MAX_WIDTH];

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

struct Board {
    int w, h;
    char board[MAX_WIDTH][MAX_HEIGHT2];

    #ifdef USE_RAND
    mutable int size[MAX_WIDTH];
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

    ShapeList(char tabuColor):
        tabuColor(tabuColor) {
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

    void send(int height) const {
        cout << "Y" << endl;
        for (int i = 0; i < nmoves; i++) {
            cout << (height - moves[i].y) << " " << (moves[i].x - 1) << endl;
        }
        cout << "-1 -1" << endl;
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

struct Color {
    char c;
    int count;
};

int colorComparator(const void* va, const void* vb) {
    return ((const Color*) vb)->count - ((const Color*) va)->count;
}

Color colorHistogram[MAX_COLOR + 1];
void calcHistogram(Board *board) {
    memset(colorHistogram, 0, sizeof(colorHistogram));

    const int w = board->w;
    const int h = board->h;

    for (int c = 1; c <= MAX_COLOR; c++) {
        colorHistogram[c].c = c;
    }

    for (int x = 1; x <= w; x++) {
        const char* col = board->board[x] + 1;
        for (int y = 1; y <= h; y++) {
            if (*col) {
                colorHistogram[*col].count++;
            }
            col++;
        }
    }

    qsort(colorHistogram + 1, MAX_COLOR, sizeof(Color), colorComparator);
    int map[MAX_COLOR + 1];
    for (int c = 1; c <= MAX_COLOR; c++) {
        map[colorHistogram[c].c] = c;
    }

    for (int x = 1; x <= w; x++) {
        char* col = board->board[x];
        for (int y = 1; y <= h; y++) {
            col[y] = map[col[y]];
        }
    }
}


void playStrategy(Board *board, Strategy& strategy, Game &game) {
    ShapeList list(strategy.getTabu());
    list.update(board);
    #ifdef VALIDATION
    validate(board, list);
    #endif
    game.reset();
    while(!list.isEmpty()) {
        const Shape* move = strategy.findBest(list);
        if (!move) {
            list.print();
        }
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

Game* compare(Board *board, vector<Strategy*>& strategies) {
    static Game games[NCOMP];
    int bestGame = 0;
    long bestScore = -1;
    for (int i = 0; i < strategies.size(); i++) {
        Board board2 = *board;
        g_seed = 1000 + i;
        playStrategy(&board2, *strategies[i], games[i]);
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
    if (colorHistogram[6].count) {
        return NULL;
    }

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

    for (int c = 1; colorHistogram[c].count && c <= MAX_COLOR; c++) {
        strategies.push_back(new ByAreaWithTabu(c));
    }
    for (int c = 1; colorHistogram[c].count && c <= 4; c++) {
        strategies.push_back(new FromTopWithTabu(c));
    }
    for (int c = 1; colorHistogram[c].count && c <= 4; c++) {
        strategies.push_back(new ByWidthWithTabu(c));
    }
    strategies.push_back(new ByAreaWithTabu(1));
    for (int c = 1; colorHistogram[c].count && c <= 3; c++) {
        strategies.push_back(new ByAreaWithTabu(c));
    }
    strategies.push_back(new ByColorAndArea());

    Game* best = compare(board, strategies);

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

void play() {
    int t;
    cin >> t;
    for (int i = 0; i < t; i++) {
        Board board;
        board.loadFromCin();
        Game* game = compare(&board);
        if (game) {
            game->send(board.h);
        } else {
            cout << "N" << endl;
        }
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
    for (int ncols = 6; ncols <= 20; ncols += 2) {
        long total[NCOMP + 3] = {0};
        for (int i = 0; i < 1000; i++) {
            int width = (rand() % 47) + 4;
            int height = (rand() % 47) + 4;
            Board* board = Board::randomBoard(width, height, ncols);
            /*for (int c = 0; c < NCOMP; c++) {
                Board board2 = *board;
                Game game;
                playStrategy(&board2, comparators[c], game);
                total[c] += game.total;
            }*/
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
            Game *game = compare(board, strategies);
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

void optimalSet2(int ncols) {
    vector<Strategy*> strategies;
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
            for (int i = 0; i < 100000; i++) {
                int width = 20;//(random() % 47) + 4;
                int height = 5;//(random() % 47) + 4;
                Board* board = Board::randomBoard(width, height, ncols);
                calcHistogram(board);
                Game *game = compare(board, strategies);
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
    //stats();
    //testFill();
    //optimalSet2(20);
    play();
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

/*high limit:
6 14874892
8 3398896
10 1347278
12 907050
14 720722
16 558472
18 478410
20 407034
1728293
1513.77
*/
