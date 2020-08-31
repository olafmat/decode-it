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
#include <limits.h>
#include "dirent.h"

#define MAX_WIDTH 52
#define MAX_HEIGHT 52
#define MAX_HEIGHT2 64
#define MAX_COLOR 20
#define MAX_GAMES 500
//#define STATS
//#define VALIDATION

using namespace std;

#ifdef STATS
int npositions = 0;
int nvariants = 0;
#endif

struct Move {
    int8_t x, y;

    void print() const {
        cout << int(x) << "x" << int(y) << endl;
    }
};

struct Shape {
    int8_t minX, maxX, minY, maxY;
    int16_t size;
    int16_t area;
    char c;
    int8_t y;
    bool vs;
    int8_t rand;

    int score() const noexcept {
        return size * (size - 1);
    }

    void print() const noexcept {
        cout << "minX=" << (int)minX << " maxX=" << (int)maxX << " minY=" << (int)minY << " maxY=" << (int)maxY <<
            " y=" << (int)y << " size=" << (int)size << " c=" << char('A' + c) << " score=" << score() <<
            " vs=" << (int)vs << endl;
    }
};

struct Segment {
    uint8_t x, minY, maxY1;

    static int comparator(const void *va, const void *vb) noexcept {
        const Segment* __restrict__ a = (const Segment*) va;
        const Segment* __restrict__ b = (const Segment*) vb;
        if (a->x != b->x) {
            return a->x - b->x;
        }
        return a->maxY1 - b->maxY1;
    }
};

struct Board {
    int w, h;
    char board[MAX_WIDTH][MAX_HEIGHT2];
    int colorHistogram[MAX_COLOR + 1];
    long maxPossibleScore;
    mutable uint64_t used[MAX_WIDTH];
    int16_t maxShapeSize;

    void operator= (const Board& src) noexcept {
        w = src.w;
        h = src.h;
        for (int x = 0; x <= w + 1; x++) {
            memcpy(board[x], src.board[x], h + 2);
        }
        //memcpy(colorHistogram, src.colorHistogram, sizeof(colorHistogram));
        //maxPossibleScore = src.maxPossibleScore;
    }

    void addFrame() noexcept {
        for (int x = 0; x <= w + 1; x++) {
            board[x][0] = board[x][h + 1] = 0;
        }
        for (int y = 1; y <= h; y++) {
            board[0][y] = board[w + 1][y] = 0;
        }
    }

    void sortColors() noexcept {
        struct Color {
            char c;
            int count;

            static int comparator(const void* va, const void* vb) noexcept {
                return ((const Color*) vb)->count - ((const Color*) va)->count;
            }
        };

        Color hist[MAX_COLOR + 1];
        memset(hist, 0, sizeof(hist));

        for (int c = 1; c <= MAX_COLOR; c++) {
            hist[c].c = c;
        }

        for (int x = 1; x <= w; x++) {
            const char* __restrict__ col = board[x] + 1;
            for (int y = 1; y <= h; y++) {
                if (*col) {
                    hist[*col].count++;
                }
                col++;
            }
        }

        qsort(hist + 1, MAX_COLOR, sizeof(Color), Color::comparator);
        int map[MAX_COLOR + 1];
        for (int c = 1; c <= MAX_COLOR; c++) {
            map[hist[c].c] = c;
            colorHistogram[c] = hist[c].count;
        }

        for (int x = 1; x <= w; x++) {
            char* __restrict__ col = board[x];
            for (int y = 1; y <= h; y++) {
                col[y] = map[col[y]];
            }
        }

        maxShapeSize = w * h / 5;
        setMaxPossibleScore();
    }

    void setMaxPossibleScore() noexcept {
        maxPossibleScore = 0;
        for (int c = 1; c <= MAX_COLOR; c++) {
            int size = colorHistogram[c];
            if (size > maxShapeSize) {
                size = maxShapeSize;
            }
            maxPossibleScore += size * (size - 1);
        }
    }

    void loadFromCin() noexcept {
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

    static Board* randomBoard(const int w, const int h, const int c) noexcept {
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

    void print() const noexcept {
        cout << "Board " << int(w) << "x" << int(h) << endl;
        for (int y = h; y > 0; y--) {
            for (int x = 1; x <= w; x++) {
                cout << (board[x][y] ? char('A' + board[x][y]) : '-');
            }
            cout << endl;
        }
    }

    void addSegment(const int x, const int minY, const int maxY1, const bool left, const bool right, const char c,
            Segment*& segments) const noexcept {
        const char *col = board[x];
        int nminY = minY;
        if (col[nminY] == c) {
            const char* __restrict__ col1 = col - 1;
            while (col1[nminY] == c) {
                nminY--;
            }
        } else {
            do {
                nminY++;
                if (nminY >= maxY1) {
                    return;
                }
            } while (col[nminY] != c);
        }

        int nmaxY1 = nminY;
        do {
            nmaxY1++;
        } while (col[nmaxY1] == c);

        if (maxY1 - nmaxY1 >= 2) {
            addSegment(x, nmaxY1 + 1, maxY1, left, right, c, segments);
        }

        const uint64_t mask = (UINT64_C(1) << nmaxY1) - (UINT64_C(1) << nminY);
        if ((used[x] & mask) != UINT64_C(0)) {
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

    void remove(const Shape& shape) noexcept {
        memset(used + shape.minX, 0, sizeof(uint64_t) * (shape.maxX - shape.minX + 1));
        Segment segments[MAX_WIDTH * MAX_HEIGHT];
        Segment *segm = segments;
        addSegment(shape.minX, shape.y, shape.y + 1, true, true, shape.c, segm);
        qsort(segments, segm - segments, sizeof(Segment), Segment::comparator);
        segm->x = MAX_WIDTH + 1;

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
            const char* src = col + s->maxY1;
            if (*src) {
                const int nminY = newLine ? h + 1 : s[1].minY;
                const int len = nminY - s->maxY1;
                memcpy(dest, src, len);
                dest += len;
            }
            if (newLine) {
                while (*dest) {
                    *dest = 0;
                    dest++;
                }
                //memset(dest, 0, h + 1 - (dest - col));
            }
            s++;
        }
    }

    int remove2(const Move move) noexcept {
        memset(used, 0, sizeof(uint64_t) * (w + 2));
        Segment segments[MAX_WIDTH * MAX_HEIGHT];
        Segment *segm = segments;
        const char c = board[move.x][move.y];
        addSegment(move.x, move.y, move.y + 1, true, true, c, segm);
        qsort(segments, segm - segments, sizeof(Segment), Segment::comparator);
        segm->x = MAX_WIDTH + 1;

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
            const char* src = col + s->maxY1;
            if (*src) {
                const int nminY = newLine ? h + 1 : s[1].minY;
                const int len = nminY - s->maxY1;
                memcpy(dest, src, len);
                dest += len;
            }
            if (newLine) {
                while (*dest) {
                    *dest = 0;
                    dest++;
                }
                //memset(dest, 0, h + 1 - (dest - col));
            }
            s++;
        }
        return size * (size - 1);
    }

    void updateHistogram(const char c, const int size) noexcept {
        const int score = size * (size - 1);

        int oldColorSize = colorHistogram[c];
        int newColorSize = oldColorSize - size;
        colorHistogram[c] = newColorSize;

        if (oldColorSize > maxShapeSize) {
            oldColorSize = maxShapeSize;
        }
        if (newColorSize > maxShapeSize) {
            newColorSize = maxShapeSize;
        }

        maxPossibleScore += score + newColorSize * (newColorSize - 1) - oldColorSize * (oldColorSize - 1);
    }

    #ifdef VALIDATION
    bool validateMove(Move &move) noexcept {
        char c = board[move.x][move.y];
        if (c == 0) {
            cout << "bad move A " << endl;
            move.print();
            print();
            return false;
        }
        if (board[move.x - 1][move.y] != c &&
            board[move.x][move.y - 1] != c &&
            board[move.x + 1][move.y] != c &&
            board[move.x][move.y + 1] != c) {
            cout << "bad move B " << endl;
            move.print();
            print();
            return false;
        }
        return true;
    }

    bool validateMove(Shape &shape) noexcept {
        if (shape.score() > 6250000) {
            shape.print();
            return false;
        }
        Move move;
        move.x = shape.minX;
        move.y = shape.y;
        bool ok = validateMove(move);
        if (!ok) {
            shape.print();
        }
        return ok;
    }
    #endif
};

struct ShapeList {
    int size[2][2];
    Shape shapes[2][2][MAX_WIDTH * MAX_HEIGHT];
    char tabuColor;
    mutable int seed;
    mutable uint64_t used[MAX_WIDTH];

    inline int fastRand() const noexcept {
      seed = (214013 * seed + 2531011);
      return (seed >> 16) & 0x7FFF;
    }

    ShapeList(const char tabuColor = 0) noexcept:
        tabuColor(tabuColor),
        seed(5325353) {
        size[0][0] = size[0][1] = size[1][0] = size[1][1] = 0;
    }

    inline bool isEmpty() const noexcept {
        return !size[0][0] && !size[0][1] && !size[1][0] && !size[1][1];
    }

    void operator=(const ShapeList& src) noexcept {
        size[0][0] = src.size[0][0];
        size[0][1] = src.size[0][1];
        size[1][0] = src.size[1][0];
        size[1][1] = src.size[1][1];
        //memcpy(size, src.size, sizeof(size));
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                memcpy(shapes[a][b], src.shapes[a][b], size[a][b] * sizeof(Shape));
            }
        }
        tabuColor = src.tabuColor;
    }

    int addSegment(const Board* const board, const int x, const int minY, const int maxY1,
            const bool left, const bool right, Shape* const dest) const noexcept {
        const char c = dest->c;
        const char *col = board->board[x];
        int nminY = minY;
        if (col[nminY] == c) {
            const char* __restrict__ col1 = col - 1;
            while (col1[nminY] == c) {
                nminY--;
            }
        } else {
            do {
                nminY++;
                if (nminY >= maxY1) {
                    return 0;
                }
            } while (col[nminY] != c);
        }

        int nmaxY1 = nminY;
        do {
            nmaxY1++;
        } while (col[nmaxY1] == c);

        int total = 0;
        if (maxY1 - nmaxY1 >= 2) {
            total += addSegment(board, x, nmaxY1 + 1, maxY1, left, right, dest);
        }

        const uint64_t mask = (UINT64_C(1) << nmaxY1) - (UINT64_C(1) << nminY);
        if ((used[x] & mask) != UINT64_C(0)) {
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

    void addShape(const Board* const board, const int x, const int y, Shape* const dest) const noexcept {
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
        dest->vs = dest->maxX == dest->minX && !board->board[dest->minX][dest->maxY + 1];
        dest->rand = fastRand();
    }

    void update(const Board * const board, int minX = 1, int maxX = MAX_WIDTH, int minY = 1) noexcept {
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
                const char c = *col;
                if (!c) {
                    break;
                }
                if (!((*usedCol >> y) & 1) &&
                        (pcol[y] == c || ncol[y] == c ||
                         col[-1] == c || col[1] == c))  {
                    const bool isTabu = c == tabuColor;
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

    int update2(const Board* const board, int minX = 1, int maxX = MAX_WIDTH, int minY = 1) noexcept {
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
                Shape* const shapes2 = shapes[a][b];
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
        const uint64_t* usedCol = used + minX;

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
                    const bool isTabu = c == tabuColor;
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

    long score() const noexcept {
        long total = 0;
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                const Shape* __restrict__ shapes2 = shapes[a][b];
                for (int i = size[a][b] - 1; i >= 0; i--) {
                    total += shapes2[i].score();
                }
            }
        }
        return total;
    }

    void print() const noexcept {
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

    void reset() noexcept {
        total = 0;
        nmoves = 0;
    }

    void operator=(const Game& src) noexcept {
        total = src.total;
        nmoves = src.nmoves;
        memcpy(moves, src.moves, nmoves * sizeof(Move));
    }

    void move(const Shape& shape) noexcept {
        total += shape.score();
        #ifdef VALIDATION
        if (total > 6250000) {
            cout << "TOTAL " << total << endl;
            shape.print();
            exit(0);
        }
        if (nmoves > MAX_WIDTH * MAX_HEIGHT) {
            cout << "MAX " << nmoves << endl;
            exit(0);
        }
        #endif
        moves[nmoves].x = shape.minX;
        moves[nmoves].y = shape.y;
        nmoves++;
        #ifdef STATS
        npositions++;
        #endif
    }

    void move(const Move move, const int score) noexcept {
        total += score;
        moves[nmoves].x = move.x;
        moves[nmoves].y = move.y;
        nmoves++;
    }

    void appendOutput(char *__restrict__ & out, const int height) const noexcept {
        *(out++) = 'Y';
        *(out++) = '\n';
        for (int i = 0; i < nmoves; i++) {
            const int y = height - moves[i].y;
            if (y > 9) {
                *(out++) = '0' + (y / 10);
            }
            *(out++) = '0' + (y % 10);

            *(out++) = ' ';

            const int x = moves[i].x - 1;
            if (x > 9) {
                *(out++) = '0' + (x / 10);
            }
            *(out++) = '0' + (x % 10);

            *(out++) = '\n';
        }
        *(out++) = '-';
        *(out++) = '1';
        *(out++) = ' ';
        *(out++) = '-';
        *(out++) = '1';
    }
};

#ifdef VALIDATION
bool validate(const Board* const board, const ShapeList& list) noexcept {
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
    Strategy(const char tabuColor) noexcept:
        tabuColor(tabuColor) {
    }

    inline char getTabu() const noexcept {
        return tabuColor;
    }

    virtual void print() const noexcept = 0;

    virtual void play(Board *board, Game *game, long bestScore, int seed) noexcept = 0;

    virtual ~Strategy() noexcept {
    }
};

class EmptySlot: public Strategy {
public:
    EmptySlot() noexcept:
        Strategy(-1) {
    }

    virtual void print() const noexcept {
        cout << "EmptySlot()" << endl;
    }

    virtual void play(Board *board, Game *game, long bestScore, int seed) noexcept {
        game->reset();
    }
};

template<bool checkMax, int versions> class MultiStrategy: public Strategy {
public:
    MultiStrategy(const char tabuColor) noexcept:
        Strategy(tabuColor) {
    }

    virtual void findBest(const ShapeList& list, Shape* __restrict__ results) noexcept = 0;

    virtual void play(Board *const board, Game *game, long const bestScore, int const seed) noexcept {
        ShapeList lists[versions];
        for (int v = 0; v < versions; v++) {
            new (&lists[v]) ShapeList(tabuColor);
            if (seed) {
                lists[v].seed = seed + v * 2000;
            }
        }

        Board* boards[versions];
        boards[0] = board;
        lists[0].update(board);
        for (int v = 1; v < versions; v++) {
            boards[v] = new Board();
            *boards[v] = *board;
            lists[v].update(boards[v]);
        }

        #ifdef VALIDATION
        for (int v = 0; v < versions; v++) {
            if (!validate(boards[v], lists[v])) {
                cout << "Val 1" <<endl;
                exit(0);
            }
        }
        #endif

        game->reset();
        while (!lists[0].isEmpty()) {
            Shape moves[versions];
            findBest(lists[0], moves);

            int bestProfit = INT_MIN;
            int bestVer = 0;
            for (int v = 0; v < versions; v++) {
                #ifdef VALIDATION
                if (!boards[v]->validateMove(moves[v])) {
                    print();
                    exit(0);
                }
                #endif
                #ifdef STATS
                nvariants++;
                #endif
                boards[v]->remove(moves[v]);

                int profit = moves[v].score() + lists[v].update2(boards[v], moves[v].minX - 1, moves[v].maxX + 1, moves[v].minY - 1);
                if (profit > bestProfit) {
                    bestProfit = profit;
                    bestVer = v;
                }
            }

            #ifdef VALIDATION
            for (int v = 0; v < versions; v++) {
                if (moves[v].score() > 6250000) {
                    moves[v].print();
                }
                if (!validate(boards[v], lists[v])) {
                    cout << "Val 2" <<endl;
                    exit(0);
                }
            }
            #endif

            Shape& move = moves[bestVer];
            game->move(move);
            for (int v = 0; v < versions; v++) {
                if (v != bestVer) {
                    *boards[v] = *boards[bestVer];
                    lists[v] = lists[bestVer];
                }
            }

            if (checkMax) {
                board->updateHistogram(move.c, move.size);
                if (board->maxPossibleScore < bestScore) {
                    break;
                }
            }

            #ifdef VALIDATION
            for (int v = 0; v < versions; v++) {
                if (!validate(boards[v], lists[v])) {
                    cout << "Val 3" <<endl;
                    exit(0);
                }
            }
            #endif
        }

        for (int v = 1; v < versions; v++) {
            delete boards[v];
        }
    }
};

template<bool checkMax, int versions> class MultiByAreaWithTabu: public MultiStrategy<checkMax, versions> {
public:
    MultiByAreaWithTabu(const char tabuColor) noexcept:
        MultiStrategy<checkMax, versions>(tabuColor) {
    }

    virtual void findBest(const ShapeList& list, Shape* __restrict__ const best) noexcept {
        memset(best, 0, sizeof(best[0]) * versions);
        int16_t areas[versions];
        for (int v = 0; v < versions; v++) {
            areas[v] = -1;
        }
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                const int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* const end = shape + size;
                    while (shape != end) {
                        int v = versions - 1;
                        while (v >= 0 && (shape->area > areas[v] ||
                            (shape->area == areas[v] && shape->rand > best[v].rand))) {
                            v--;
                        }
                        v++;
                        if (v < versions) {
                            memmove(best + v + 1, best + v, (versions - v - 1) * sizeof(best[0]));
                            memmove(areas + v + 1, areas + v, (versions - v - 1) * sizeof(areas[0]));
                            best[v] = *shape;
                            areas[v] = shape->area;
                        }
                        shape++;
                    }
                    if (best[versions - 1].size) {
                        return;
                    }
                }
            }
        }
        for (int v = 1; v < versions; v++) {
            if (!best[v].size) {
                best[v] = best[0];
            }
        }
    }

    virtual void print() const noexcept {
        cout << "MultiByAreaWithTabu<" << checkMax << "," << versions << ">(" << int(Strategy::tabuColor) << ")" << endl;
    }
};

template<int versions1, int versions2> class DoubleStrategy: public Strategy {
public:
    DoubleStrategy(const char tabuColor) noexcept:
        Strategy(tabuColor) {
    }

    virtual void findBest(ShapeList& list, Shape* __restrict__ results, const int size) noexcept = 0;

    virtual void play(Board *board, Game *game, long bestScore, int seed) noexcept {
        constexpr auto versions = versions1 + versions2;

        ShapeList lists[versions];
        for (int v = 0; v < versions; v++) {
            new (&lists[v]) ShapeList(tabuColor);
            if (seed) {
                lists[v].seed = seed + v * 2000;
            }
        }

        Board* boards[versions];
        Game games[versions];
        boards[0] = board;
        lists[0].update(board);
        games[0].reset();
        for (int v = 1; v < versions; v++) {
            boards[v] = new Board();
            *boards[v] = *board;
            lists[v].update(boards[v]);
            games[v].reset();
        }

        #ifdef VALIDATION
        for (int v = 0; v < versions; v++) {
            if (!validate(boards[v], lists[v])) {
                cout << "Val 5" <<endl;
                exit(0);
            }
        }
        #endif

        while(true) {
            const bool empty1 = lists[0].isEmpty();
            const bool empty2 = lists[versions1].isEmpty();
            if (empty1 && empty2) {
                break;
            }
            Shape moves[versions];
            if (!empty1) {
                findBest(lists[0], moves, versions1);
            }
            if (!empty2) {
                findBest(lists[versions1], moves + versions1, versions2);
            }

            #ifdef VALIDATION
            for (int v = 0; v < versions; v++) {
                #ifdef VALIDATION
                if (!boards[v]->validateMove(moves[v])) {
                    cout << "move2 " << v << " " << versions1 << endl;
                    print();
                    exit(0);
                }
                #endif
                if (!validate(boards[v], lists[v])) {
                    cout << "Val 7 " << v << endl;
                    print();
                    exit(0);
                }
            }
            #endif

            int bestProfit1 = INT_MIN;
            int bestVer1 = 0;
            int bestProfit2 = INT_MIN;
            int bestVer2 = 0;
            int minVer = empty1 ? versions1 : 0;
            int maxVer = empty2 ? versions1 : versions;
            for (int v = minVer; v < maxVer; v++) {
                Shape &move = moves[v];
                #ifdef VALIDATION
                if (!boards[v]->validateMove(move)) {
                    cout << "move1 " << v << " " << minVer << " " << maxVer << " " << versions1 << endl;
                    print();
                    exit(0);
                }
                #endif
                #ifdef STATS
                nvariants++;
                #endif
                games[v].move(move);
                boards[v]->remove(move);

                int profit = move.score() + lists[v].update2(boards[v], move.minX - 1, move.maxX + 1, move.minY - 1);
                if (profit > bestProfit2) {
                    if (profit > bestProfit1) {
                        bestProfit2 = bestProfit1;
                        bestVer2 = bestVer1;
                        bestProfit1 = profit;
                        bestVer1 = v;
                    } else {
                        bestProfit2 = profit;
                        bestVer2 = v;
                    }
                }
            }

            //cout << bestVer1 << " " <<bestVer2 << " " << empty1 << empty2 << endl;
            #ifdef VALIDATION
            for (int v = 0; v < versions; v++) {
                if (!validate(boards[v], lists[v])) {
                    cout << "Val 6 " << v << endl;
                    print();
                    exit(0);
                }
            }
            #endif

            int v = 0;
            if (versions1 == 1 && !bestVer2) {
                const Board boardAux = *boards[bestVer1];
                const ShapeList listAux = lists[bestVer1];
                const Game gameAux = games[bestVer1];
                for (v = 1; v < versions; v++) {
                    //cout << "copy " << 0 << " to " << v << endl;
                    *boards[v] = *boards[0];
                    lists[v] = lists[0];
                    games[v] = games[0];
                }
                //cout << "copy saved " << bestVer1 << " to " << 0 << endl;
                *boards[0] = boardAux;
                lists[0] = listAux;
                games[0] = gameAux;
            } else {
                for (; v < versions1; v++) {
                    if (v != bestVer1 && v != bestVer2) {
                        //cout << "copy " << bestVer1 << " to " << v << endl;
                        *boards[v] = *boards[bestVer1];
                        lists[v] = lists[bestVer1];
                        games[v] = games[bestVer1];
                    }
                }
                for (; v < versions; v++) {
                    if (v != bestVer2) {
                        //cout << "copy " << bestVer2 << " to " << v << endl;
                        *boards[v] = *boards[bestVer2];
                        lists[v] = lists[bestVer2];
                        games[v] = games[bestVer2];
                    }
                }
                if (bestVer2 < versions1) {
                    const int from = bestVer2 ? 0 : 1;
                    //cout << "copy " << from << " to " << bestVer2 << endl;
                    *boards[bestVer2] = *boards[from];
                    lists[bestVer2] = lists[from];
                    games[bestVer2] = games[from];
                }
            }
            /*if (checkMax) {
                board->updateHistogram(move.c, move.size);
                if (board->maxPossibleScore < bestScore) {
                    break;
                }
            }*/

            #ifdef VALIDATION
            for (int v = 0; v < versions; v++) {
                if (!validate(boards[v], lists[v])) {
                    cout << "Val 7" <<endl;
                    exit(0);
                }
            }
            #endif
        }

        int bestGame = 0;
        long best = games[0].total;
        for (int v = 1; v < versions; v++) {
            delete boards[v];
            long score = games[v].total;
            if (score > best) {
                bestGame = v;
                best = score;
            }
        }

        *game = games[bestGame];
    }
};

template<int versions1, int versions2> class DoubleByAreaWithTabu: public DoubleStrategy<versions1, versions2> {
public:
    DoubleByAreaWithTabu(const char tabuColor) noexcept:
        DoubleStrategy<versions1, versions2>(tabuColor) {
    }

    virtual void findBest(ShapeList& list, Shape* const __restrict__ best, const int ver) noexcept {
        memset(best, 0, sizeof(best[0]) * ver);
        int16_t areas[ver];
        for (int v = 0; v < ver; v++) {
            areas[v] = -1;
        }
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    while(shape != end) {
                        int v = ver - 1;
                        while (v >= 0 && (shape->area > areas[v] ||
                            (shape->area == areas[v] && shape->rand > best[v].rand))) {
                            v--;
                        }
                        v++;
                        if (v < ver) {
                            memmove(best + v + 1, best + v, ((ver - 1) - v) * sizeof(best[0]));
                            memmove(areas + v + 1, areas + v, ((ver - 1) - v) * sizeof(areas[0]));
                            best[v] = *shape;
                            areas[v] = shape->area;
                        }
                        shape++;
                    }
                    if (best[ver - 1].size) {
                        return;
                    }
                }
            }
        }

        for (int v = 1; v < ver; v++) {
            if (!best[v].size) {
                best[v] = best[0];
            }
        }
    }

    virtual void print() const noexcept {
        cout << "DoubleByAreaWithTabu<" << versions1 << "," << versions2 << ">(" << int(Strategy::tabuColor) << ")" << endl;
    }
};

#ifdef VALIDATION
void validateGame(const Board *const board, const Game& game, bool earlyStop) noexcept {
    Board board2 = *board;
    long total = 0;
    for (int i = 0; i < game.nmoves; i++) {
        Move move = game.moves[i];
        if (!board2.validateMove(move)) {
            exit(0);
        }
        int score = board2.remove2(move);
        total += score;
    }
    if (total != game.total) {
        cout << "bad total " << total << " " << game.total << endl;
    }

    if (!earlyStop) {
        for (int x = 1; x <= board->w; x++) {
            for (int y = 1; y <= board->h; y++) {
                char c = board2.board[x][y];
                if (c != 0 &&
                    (board2.board[x - 1][y] == c ||
                    board2.board[x][y - 1] == c ||
                    board2.board[x + 1][y] == c ||
                    board2.board[x][y + 1] == c))
                {
                    cout << "unused move " << x << "x" << y << " " << char('A' + c) << endl;
                    board2.print();
                    exit(0);
                }
            }
        }
    }
}
#endif

const Game* compare(const Board *const board, vector<Strategy*>& strategies, Game* const games) noexcept {
    int bestGame = 0;
    long bestScore = -1;
    for (int i = 0; i < strategies.size(); i++) {
        Board board2 = *board;
        strategies[i]->play(&board2, &games[i], bestScore, 1000 + i);
        if (games[i].total > bestScore) {
            bestGame = i;
            bestScore = games[i].total;
        }
        #ifdef VALIDATION
        if (games[i].nmoves) {
            validateGame(board, games[i], i > 0);
        }
        #endif
    }
    #ifdef VALIDATION
    if (bestGame < 0 || bestGame >= strategies.size()) {
        cout << "wrong game " << bestGame << endl;
    }
    #endif
    return games + bestGame;
}

const Game* compare(Board *const board) noexcept {
    board->sortColors();

    vector<Strategy*> strategies;
    #define E strategies.push_back(new EmptySlot());
    #define M(v, c) \
        if (first) { \
            strategies.push_back(new MultiByAreaWithTabu<false, v>(c)); \
            first = false; \
        } else { \
            strategies.push_back(new MultiByAreaWithTabu<true, v>(c)); \
        };
    bool first = true;
    if (!board->colorHistogram[6]) {
        if (board->w < 25) {
            M(14,1) M(13,2) M(2,3) M(2,1) E E M(2,2) M(3,1)
        } else if (board->w < 35) {
            M(14,1) M(14,2) M(2,3) E M(1,2) E E M(3,1) M(8,2) M(1,1)
        } else if (board->w < 45) {
            M(7,1) M(10,2) E E E E E M(1,1) M(9,1) M(6,2)
        } else {
            M(8,1) M(5,2) M(10,4) E E E E M(10,1)
        }
    } else if (!board->colorHistogram[7]) {
        if (board->w < 25) {
            E E M(3,2) M(3,1) E M(2,2)
        } else if (board->w < 35) {
            M(7,1) M(5,2) E E E E M(1,1) M(2,1)
        } else {
            M(7,1) E M(3,3)
        }
    } else if (!board->colorHistogram[8]) {
        if (board->w < 25) {
            M(7,1) M(7,2) E E M(2,2) E E M(3,1)
        } else if (board->w < 35) {
            M(8,1) E E E E E E M(3,1)
        } else if (board->w < 45) {
            M(7,1) E E M(3,1) E E E M(3,1)
        } else {
            E E E E M(3,2) M(2,3)
        }
    } else if (!board->colorHistogram[9]) {
        if (board->w < 25) {
            M(8,1) M(4,2) M(4,3) E E E E M(3,1) M(1,4) M(1,1) M(2,1)
        } else if (board->w < 35) {
            M(7,1) M(5,2) M(3,3)
        } else if (board->w < 45) {
            M(7,1) M(5,2) M(3,3) M(3,1) E E E M(3,1) M(1,3)
        } else {
            E M(7,2) E M(3,1) M(2,2) E E E E M(1,4)
        }
    } else if (!board->colorHistogram[10]) {
        if (board->w < 25) {
            M(17,1) E E M(5,1)
        } else if (board->w < 35) {
            M(19,1) M(1,1)
        } else if (board->w < 45) {
            M(15,1)
        } else {
            M(21,1)
        }
    } else if (!board->colorHistogram[11]) {
        if (board->w < 25) {
            M(24,1)
        } else if (board->w < 35) {
            M(18,1)
        } else if (board->w < 45) {
            M(20,1)
        } else {
            E M(1,4) E M(1,2)
        }
    } else if (!board->colorHistogram[12]) {
        if (board->w < 25) {
            M(24,1)
        } else if (board->w < 35) {
            M(24,1)
        } else if (board->w < 45) {
            M(24,1)
        } else {
            M(20,1)
        }
    } else if (!board->colorHistogram[13]) {
        M(24,1) M(1,1)
        if (board->w < 25) {
            M(5,1)
        }
        M(1,1)
    } else {
        if (board->w < 25) {
            M(21,0) M(3,1) M(2,0)
        } else if (board->w < 35) {
            M(21,0) M(3,0)
        } else if (board->w < 45) {
            M(21,0) M(3,0)
        } else {
            M(21,0)
        }
    }
    #undef E
    #undef M

    Game games[strategies.size()];
    const Game* best = compare(board, strategies, games);

    for (int i = 0; i < strategies.size(); i++) {
        delete strategies[i];
    }
    return best;
}

int findBestGame(const Game* const games, const ShapeList* const lists, const int cnt) noexcept {
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

void solve(Board &board, char* buf) noexcept {
    const Game* game = compare(&board);
    char* __restrict__ out = buf;
    if (game) {
        game->appendOutput(out, board.h);
    } else {
        *(out++) = 'N';
    }
    *out = 0;
}

void play() noexcept {
    int t;
    cin >> t;

    static Board boards[MAX_GAMES];
    for (int i = 0; i < t; i++) {
        boards[i].loadFromCin();
    }

    static char out[MAX_GAMES][7 * MAX_WIDTH * MAX_HEIGHT + 10];
    for (int i = 0; i < t; i++) {
        solve(boards[i], out[i]);
    }

    for (int i = 0; i < t; i++) {
        puts(out[i]);
    }
}

void stats() noexcept {
    const int64_t begin = clock();
    //int width = 20;
    //int height = 50;
    long total2 = 0;
    int ngames = 0;
    int nmoves = 0;
    for (int ncols = 5; ncols <= 19; ncols ++) {
        long total = 0;
        for (int i = 0; i < 1000; i++) {
            int width = (rand() % 4) * 10 + 20;//(rand() % 47) + 4;
            int height = width; //(rand() % 47) + 4;
            Board* board = Board::randomBoard(width, height, ncols);
            Board board2 = *board;
            const Game *game = compare(&board2);
            if (game != NULL) {
                ngames++;
                nmoves += game->nmoves;
                long score = game->total * ncols * ncols / width / height;
                total += score;
                if (total > 6250000000L) {
                    cout << "too big " << i << " " << total << " " << game->total << endl;
                }
                total2 += score;
            }
            delete board;
        }
        cout << ncols << " " << total;
        cout << endl;
    }
    cout << total2;
    cout << endl;

    const int64_t end = clock();
    const double duration = double(end - begin) / CLOCKS_PER_SEC;
    cout << duration << endl;
    cout << "games    \t" << ngames << "\t\t" << double(ngames) / duration << " /s" << endl;
    cout << "moves    \t" << nmoves << "\t\t" << double(nmoves) / duration << " /s" << endl;
    #ifdef STATS
    cout << "positions\t" << npositions << "\t" << double(npositions) / duration << " /s" << endl;
    cout << "variants \t" << nvariants << "\t" << double(nvariants) / duration << " /s" << endl;
    #endif
}

/*void testFill() noexcept {
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

bool comboComparator(const Combo& a, const Combo& b) noexcept {
    int64_t diff = a.time - b.time;
    if (diff < 0) {
        return true;
    }
    if (diff > 0) {
        return false;
    }
    return a.result > b.result;
}

void optimalSet(int ncols) noexcept {
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
        //for (int c = 1; c <= w1; c++) {
          //  strategies.push_back(new ByWidthWithTabu(c));
        }//
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
            board->sortColors();
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

void optimalSet2(int ncols, int width) noexcept {
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
                board->sortColors();
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
}*/

/*void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 3);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}*/


int main() noexcept {
    //signal(SIGSEGV, handler);
    //signal(SIGBUS, handler);
    //stats();
    //testFill();
    //optimalSet2(5, 30);
    play();
    //randomPlay();
    return 0;
}

