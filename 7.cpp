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
//#define USE_RAND_STRATEGY
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
    int16_t area;
    char c;
    int8_t y;
    char vs;
    #ifdef USE_RAND
    char rand;
    #endif

    int score() const {
        return size * (size - 1);
    }

    void print() const {
        cout << "minX=" << (int)minX << " maxX=" << (int)maxX << " minY=" << (int)minY << " maxY=" << (int)maxY <<
            " y=" << (int)y << " size=" << (int)size << " c=" << char('A' + c) << " score=" << score() << endl;
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
    int size;
    Shape shapes[MAX_WIDTH * MAX_HEIGHT];

    ShapeList(): size(0) {
    }

    void operator=(const ShapeList& src) {
        size = src.size;
        memcpy(shapes, src.shapes, size * sizeof(Shape));
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
        #ifdef USE_RAND_STRATEGY
        dest->rand = random();
        #endif
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
                    addShape(board, x, y, dest);
                    dest++;
                }
                col++;
            }
            usedCol++;
        }
        size = dest - shapes;
    }

    long score() const {
        long total = 0;
        for (int i = 0; i < size; i++) {
            total += shapes[i].score();
        }
        return total;
    }

    const Shape& findBest(bool (*comparator)(const Shape*, const Shape*)) const {
        const Shape* best = &shapes[0];
        for (int i = 1; i < size; i++) {
            if (comparator(&shapes[i], best)) {
                best = &shapes[i];
            }
        }
        return *best;
    }

    void print() const {
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

bool fromSmallest(const Shape *a, const Shape *b) {
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
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    return a->minX < b->minX;
}

template<int chosenOne> bool fromSmallestWithoutOne(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca < cb;
    }
    return a->size < b->size;
}

template<int chosenOne> bool fromTopWithoutOne(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca < cb;
    }
    return b->y < a->y;
}

template<int chosenOne> bool fromWidestWithoutOne(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
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
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    int ca = a->c == chosenOne;
    int cb = b->c == chosenOne;
    if (ca != cb) {
        return ca < cb;
    }
    return b->area < a->area;
}

bool byColorAndFromSmallest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return a->size < b->size;
}

bool byColorAndFromLargest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->size < a->size;
}

bool byColorDescAndFromLargest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    if (a->c != b->c) {
        return a->c < b->c;
    }
    return b->size < a->size;
}

bool byColorAndFromWidest(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
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
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->area < a->area;
}

bool byColorAndFromTop(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->y < a->y;
}

bool byColorAndFromTop2(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->minY < a->minY;
}

bool byColorAndFromTop3(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    if (a->c != b->c) {
        return b->c < a->c;
    }
    return b->maxY < a->maxY;
}

#ifdef USE_RAND_STRATEGY
bool randomStrategy(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs < b->vs;
    }
    return a->rand < b->rand;
}
#endif

#ifdef VALIDATION
bool validate(const Board *board, const ShapeList& list) {
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


void test(Board *board, bool (*comparator)(const Shape*, const Shape*), Game &game) {
    ShapeList list;
    list.update(board);
    #ifdef VALIDATION
    validate(board, list);
    #endif

    game.reset();
    while(list.size) {
        const Shape& move = list.findBest(comparator);
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

/*const int NCOMP = 9;
bool (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromSmallest, fromLargest, fromBottom, fromTop, fromLeft, fromSmallestWithoutOne, byColorAndFromSmallest,
    byColorAndFromLargest, byColorAndFromTop
};*/
const int NCOMP_ = 18;
bool (*comparators_[NCOMP_])(const Shape*, const Shape*) = {
    /*fromSmallest, */fromLargest, /*fromBottom, */fromTop, /*fromBottom2, */fromTop2, /*fromBottom3, */fromTop3, /*fromLeft,*/
    fromSmallestWithoutOne<1>, fromSmallestWithoutOne<2>, fromSmallestWithoutOne<3>, fromSmallestWithoutOne<4>,
    fromTopWithoutOne<1>, fromTopWithoutOne<2>, fromTopWithoutOne<3>, fromTopWithoutOne<4>,
    byColorAndFromSmallest,
    byColorAndFromLargest, byColorDescAndFromLargest, byColorAndFromTop, byColorAndFromTop2, byColorAndFromTop3
};
const int NCOMP = 28;
bool (*comparators[NCOMP])(const Shape*, const Shape*) = {
    byAreaWithoutOne<1>, byAreaWithoutOne<2>, byAreaWithoutOne<3>, byAreaWithoutOne<4>,
    byAreaWithoutOne<5>, byAreaWithoutOne<6>, byAreaWithoutOne<7>, byAreaWithoutOne<8>,
    byAreaWithoutOne<9>, byAreaWithoutOne<10>, byAreaWithoutOne<11>, byAreaWithoutOne<12>,
    byAreaWithoutOne<13>, byAreaWithoutOne<14>, byAreaWithoutOne<15>, byAreaWithoutOne<16>,
    byAreaWithoutOne<17>, byAreaWithoutOne<18>, byAreaWithoutOne<19>, byAreaWithoutOne<20>,
    fromTopWithoutOne<1>, fromTopWithoutOne<2>, fromTopWithoutOne<3>, fromTopWithoutOne<4>,
    //fromTopWithoutOne<5>, fromTopWithoutOne<6>,
    fromWidestWithoutOne<1>, fromWidestWithoutOne<2>, fromWidestWithoutOne<3>, //fromWidestWithoutOne<4>,
    //fromWidestWithoutOne<5>, //fromWidestWithoutOne<6>, fromTopWithoutOne<7>, fromTopWithoutOne<8>,
    //fromTopWithoutOne<9>, fromTopWithoutOne<10>, //fromTopWithoutOne<11>, fromTopWithoutOne<12>,
    //fromTopWithoutOne<13>, fromTopWithoutOne<14>, //fromTopWithoutOne<15>, fromTopWithoutOne<16>,
    //fromTopWithoutOne<17>, fromTopWithoutOne<18>, fromTopWithoutOne<19>, fromTopWithoutOne<20>
    /*byColorAndFromWidest,*/ byColorAndArea
};
/*const int NCOMP = 23;
bool (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromSmallest, fromLargest, fromBottom, fromTop, fromBottom2, fromTop2, fromBottom3, fromTop3, fromLeft,
    fromSmallestWithoutOne<1>, fromLargestWithoutMostPop,
    fromSmallestWithoutMostPop, byColorAndFromSmallest,
    byColorAndFromLargest, byColorDescAndFromLargest, byColorAndFromTop, byColorAndFromTop2, byColorAndFromTop3,
    byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop, byColorNoAndFromTop2, byColorNoAndFromTop3
};*/
/*const int NCOMP = 3;
bool (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromTop, byColorAndFromLargest, byColorAndFromTop
};*/
//const int NCOMP2 = 10;
//bool (*comparators2[NCOMP2])(const Shape*, const Shape*) = {
//    fromLargest, fromTop, /*fromSmallestWithoutOne, */fromLargestWithoutMostPop, fromSmallestWithoutMostPop, byColorAndFromSmallest,
//    byColorAndFromLargest, byColorAndFromTop, byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop
//};
const int NCOMP2 = NCOMP;
bool (**comparators2)(const Shape*, const Shape*) = comparators;
/*const int NCOMP2 = 6;
bool (*comparators2[NCOMP2])(const Shape*, const Shape*) = {
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
        if (i == NCOMP - 1 || colorHistogram[(i < 20 ? i : i < 24 ? i - 20 : i - 24) + 1].count) {
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
            hist2[bestGame]++;
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

const int NRAND = 1009;
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
    for (int ncols = 6; ncols <= 20; ncols += 2) {
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
//1565995 72.6904 - 2978.1 2.8      16A 4T 3W BCA
//1566722 74.6411 - 2979   2.9      20A 4T 3W BCA
