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

    ShapeList(char tabuColor = 0):
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

    void shuffle() {
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                Shape *sh = shapes[a][b];
                Shape *end = sh + size[a][b];
                while (sh != end) {
                    sh->rand = fastRand();
                    sh++;
                }
            }
        }
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

class DualStrategy: public Strategy {
public:
    DualStrategy(char tabuColor): Strategy(tabuColor) {
    }

    virtual const Shape* findBest(ShapeList& list) {
        return NULL;
    }

    virtual void findBest2(ShapeList& list, Shape* results) = 0;

    virtual void play(Board *board1, Game &game, int seed = 0) {
        ShapeList list1(tabuColor);
        ShapeList list2(tabuColor);
        if (seed) {
            list1.g_seed = seed;
            list2.g_seed = seed + 2000;
        }

        Board board2 = *board1;
        list1.update(board1);
        list2.update(&board2);

        #ifdef VALIDATION
        validate(board1, list1);
        validate(&board2, list2);
        #endif

        game.reset();
        while(!list1.isEmpty()) {
            Shape moves[2];
            findBest2(list1, moves);

            board1->remove(moves[0]);
            int profit1 = moves[0].score() + list1.update2(board1, moves[0].minX - 1, moves[0].maxX + 1, moves[0].minY - 1);
            board2.remove(moves[1]);
            int profit2 = moves[1].score() + list2.update2(&board2, moves[1].minX - 1, moves[1].maxX + 1, moves[1].minY - 1);

            #ifdef VALIDATION
            if (moves[0].score() > 6250000) {
                moves[0].print();
            }
            validate(board1, list1);
            if (moves[1].score() > 6250000) {
                moves[1].print();
            }
            validate(&board2, list2);
            #endif

            if (profit1 >= profit2) {
                game.move(moves[0]);
                board2 = *board1;
                list2 = list1;
                //list2.shuffle();
            } else {
                game.move(moves[1]);
                *board1 = board2;
                list1 = list2;
                //list1.shuffle();
            }
            #ifdef VALIDATION
            validate(board1, list1);
            validate(&board2, list2);
            #endif
        }
    }
};

class DualByAreaWithTabu: public DualStrategy {
public:
    DualByAreaWithTabu(char tabuColor): DualStrategy(tabuColor) {
    }

    virtual void findBest2(ShapeList& list, Shape* results) {
        const Shape* best1 = NULL;
        const Shape* best2 = NULL;
        int16_t area1 = -1;
        int16_t area2 = -1;
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    while(shape != end) {
                        if (shape->area > area2 || (shape->area == area2 && shape->rand > best2->rand)) {
                            if (shape->area > area1 || (shape->area == area1 && shape->rand > best1->rand)) {
                                best2 = best1;
                                area2 = area1;
                                best1 = shape;
                                area1 = shape->area;
                            } else {
                                best2 = shape;
                                area2 = shape->area;
                            }
                        }
                        shape++;
                    }
                    if (best2) {
                        results[0] = *best1;
                        results[1] = *best2;
                        return;
                    }
                }
            }
        }
        results[0] = *best1;
        results[1] = best2 ? *best2 : *best1;
    }

    virtual void print() const {
        cout << "DualByAreaWithTabu(" << int(tabuColor) << ")" << endl;
    }
};

class DualByWidthWithTabu: public DualStrategy {
public:
    DualByWidthWithTabu(char tabuColor): DualStrategy(tabuColor) {
    }

    virtual void findBest2(ShapeList& list, Shape* results) {
        const Shape* best1 = NULL;
        const Shape* best2 = NULL;
        int8_t width1 = -1;
        int8_t width2 = -1;
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    while(shape != end) {
                        const int8_t width = shape->maxX - shape->minX;
                        if (width > width2 || (width == width2 && shape->rand > best2->rand)) {
                            if (width > width1 || (width == width1 && shape->rand > best1->rand)) {
                                best2 = best1;
                                width2 = width1;
                                best1 = shape;
                                width1 = width;
                            } else {
                                best2 = shape;
                                width2 = width;
                            }
                        }
                        shape++;
                    }
                    if (best2) {
                        results[0] = *best1;
                        results[1] = *best2;
                        return;
                    }
                }
            }
        }
        results[0] = *best1;
        results[1] = best2 ? *best2 : *best1;
    }

    virtual void print() const {
        cout << "DualByWidthWithTabu(" << int(tabuColor) << ")" << endl;
    }
};

template<int versions> class MultiStrategy: public Strategy {
public:
    MultiStrategy(char tabuColor): Strategy(tabuColor) {
    }

    virtual const Shape* findBest(ShapeList& list) {
        return NULL;
    }

    virtual void findBest2(ShapeList& list, Shape* results) = 0;

    virtual void play(Board *board, Game &game, int seed = 0) {
        ShapeList lists[versions];
        for (int v = 0; v < versions; v++) {
            new (&lists[v]) ShapeList(tabuColor);
            if (seed) {
                lists[v].g_seed = seed + v * 2000;
            }
        }

        Board* boards[versions] = {board};
        lists[0].update(board);
        for (int v = 1; v < versions; v++) {
            boards[v] = new Board();
            *boards[v] = *board;
            lists[v].update(boards[v]);
        }

        #ifdef VALIDATION
        for (int v = 0; v < versions; v++) {
            validate(boards[v], lists[v]);
        }
        #endif

        game.reset();
        while(!lists[0].isEmpty()) {
            Shape moves[versions];
            findBest2(lists[0], moves);

            int bestProfit = INT_MIN;
            int bestVer = 0;
            for (int v = 0; v < versions; v++) {
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
                validate(boards[v], lists[v]);
            }
            #endif

            game.move(moves[bestVer]);
            for (int v = 0; v < versions; v++) {
                if (v != bestVer) {
                    *boards[v] = *boards[bestVer];
                    lists[v] = lists[bestVer];
                    // lists[v].shuffle();
                }
            }
            #ifdef VALIDATION
            for (int v = 0; v < versions; v++) {
                validate(boards[v], lists[v]);
            }
            #endif
        }
        
        for (int v = 1; v < versions; v++) {
            delete boards[v];
        }
    }
};

template<int versions> class MultiByAreaWithTabu: public MultiStrategy<versions> {
public:
    MultiByAreaWithTabu(char tabuColor): MultiStrategy<versions>(tabuColor) {
    }

    virtual void findBest2(ShapeList& list, Shape* best) {
        memset(best, 0, sizeof(best[0]) * versions);
        int16_t areas[versions];
        for (int v = 0; v < versions; v++) {
            areas[v] = -1;
        }
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                int size = list.size[a][b];
                if (size) {
                    const Shape* shape = list.shapes[a][b];
                    const Shape* end = shape + size;
                    while(shape != end) {
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

    virtual void print() const {
        cout << "MultiByAreaWithTabu<" << versions << ">(" << int(Strategy::tabuColor) << ")" << endl;
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
    if (!board->colorHistogram[9].count) {
        strategies.push_back(new MultiByAreaWithTabu<7>(1));
        strategies.push_back(new MultiByAreaWithTabu<5>(2));
        strategies.push_back(new MultiByAreaWithTabu<3>(3));
        strategies.push_back(new MultiByAreaWithTabu<3>(1));
        strategies.push_back(new MultiByAreaWithTabu<3>(2));
        strategies.push_back(new MultiByAreaWithTabu<2>(3));
        strategies.push_back(new MultiByAreaWithTabu<1>(1));
        strategies.push_back(new MultiByAreaWithTabu<1>(1));
    } else {
        strategies.push_back(new MultiByAreaWithTabu<21>(1));
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

/*void testFill() {
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
*/

int main() {
    //signal(SIGSEGV, handler);
    //signal(SIGBUS, handler);
    //stats();
    //testFill();
    //optimalSet2(5, 30);
    play();
    //randomPlay();
    return 0;
}

