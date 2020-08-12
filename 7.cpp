#include <stdlib.h>
#include <memory.h>
#include <iostream>

#define MAX_WIDTH 50
#define MAX_HEIGHT 50
#define MAX_HEIGHT2 64
#define MAX_COLOR 20

using namespace std;

struct Shape;
struct ShapeList;

uint64_t used[MAX_WIDTH];

struct Move {
    int x, y;
};

struct Shape {
    int8_t minX, maxX, minY, maxY;
    int16_t size;
    char c;
    int8_t y;
    char vs;
    char rand;

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
    int size[MAX_WIDTH];

    void operator= (const Board& src) {
        w = src.w;
        h = src.h;
        for (int j = 0; j < w; j++) {
            memcpy(board[j], src.board[j], h);
        }
    }

    void loadFromCin() {
        int nc;
        cin >> h >> w >> nc;
        for (int y = h - 1; y >= 0; y--) {
            for (int x = 0; x < w; x++) {
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

    void addPoint(int x, int y, char c) {
        const uint64_t mask = uint64_t(1) << y;
        if ((used[x] & mask) == uint64_t(0) && board[x][y] == c) {
            used[x] |= mask;
            if (x > 0) {
                addPoint(x - 1, y, c);
            }
            if (x < w - 1) {
                addPoint(x + 1, y, c);
            }
            if (y > 0) {
                addPoint(x, y - 1, c);
            }
            if (y < h - 1) {
                addPoint(x, y + 1, c);
            }
        }
    }

    void remove(Shape& shape) {
        //cout << "remove ";
        //shape.print();
        memset(used + shape.minX, 0, sizeof(uint64_t) * (shape.maxX - shape.minX + 1));
        addPoint(shape.minX, shape.y, shape.c);
        for (int x = shape.minX; x <= shape.maxX; x++) {
            uint64_t mask = used[x] >> shape.minY;
            char* src = board[x] + shape.minY;
            char* dest = src;
            for (int y = shape.minY; y < h; y++) {
                if ((mask & 1) == uint64_t(0)) {
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
    }

    int addPoint2(int x, int y, char c) {
        const uint64_t mask = uint64_t(1) << y;
        int total = 0;
        if ((used[x] & mask) == uint64_t(0) && board[x][y] == c) {
            used[x] |= mask;
            total++;
            if (x > 0) {
                total += addPoint2(x - 1, y, c);
            }
            if (x < w - 1) {
                total += addPoint2(x + 1, y, c);
            }
            if (y > 0) {
                total += addPoint2(x, y - 1, c);
            }
            if (y < h - 1) {
                total += addPoint2(x, y + 1, c);
            }
        }
        return total;
    }

    int remove2(Move move) {
        memset(used, 0, sizeof(uint64_t) * w);
        int size = addPoint2(move.x, move.y, board[move.x][move.y]);
        if (!size) {
            return 0;
        }
        int x = 0;
        while(used[x] == uint64_t(0)) {
            x++;
        }
        for (; x < w; x++) {
            uint64_t mask = used[x];
            if (mask == uint64_t(0)) {
                break;
            }
            char* src = board[x];
            char* dest = src;
            for (int y = 0; y < h; y++) {
                if ((mask & 1) == uint64_t(0)) {
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
            if (y < dest->minY) {
                dest->minY = y;
            }
            if (y > dest->maxY) {
                dest->maxY = y;
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
        dest->minY = board->h;
        dest->maxY = -1;
        dest->c = board->board[x][y];
        dest->size = addPoint(board, x, y, dest);
        dest->vs = (dest->maxX == dest->minX && (dest->maxY >= board->h - 1 || !board->board[dest->minX][dest->maxY + 1]))/* ||
            !(dest->maxX == dest->minX && dest->minY > 0 && dest->maxY < board->h - 1 &&
                board->board[dest->minX][dest->minY - 1] == board->board[dest->minX][dest->maxY + 1])*/;
        dest->rand = random();
    }

    void update(Board *board, int minX = 0, int maxX = MAX_WIDTH, int minY = 0) {
//cout << __LINE__ << " " << minX << " " << maxX << " " << minY << endl;
        if (minX < 0) {
            minX = 0;
        }
        const int w = board->w;
        const int h = board->h;
        if (maxX >= w) {
            maxX = w - 1;
        }
        if (minY < 0) {
            minY = 0;
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

//cout << __LINE__ << " " << minX << " " << maxX << " " << minY << endl;
        memset(used, 0, sizeof(uint64_t) * w);
//cout << __LINE__ << endl;
        uint64_t* usedCol = used + minX;
//cout << __LINE__ << endl;

        for (int x = minX; x <= maxX; x++) {
            char* col = &board->board[x][0/*minY*/];
            /*for (int x2 = 0; x2 < w; x2++)
                cout << " " << used[x2];
            cout<< endl;*/
            for (int y = 0/*minY*/; y < h; y++) {
                //cout << "check " << x << " " << y << " col=" << char('A' + *col) << " " << *usedCol << " " << ((*usedCol >> y) & 1) << endl;
                if (*col && !((*usedCol >> y) & 1)) {
                    addShape(board, x, y, dest);
        //cout << __LINE__ << " " << x << " " << y << " " << dest-shapes << endl;
                    if (dest->size > 1) {
                        dest++;
                    }
                }
                col++;
            }
            usedCol++;
        }
        size = dest - shapes;
//cout << __LINE__ << endl;
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

    Shape& findBest(int (*comparator)(const Shape*, const Shape*)) {
//cout << __LINE__ << " " << (void*)comparator << " " << size << endl;
        Shape* best = &shapes[0];
        for (int i = 1; i < size; i++) {
//cout << __LINE__ << " " << comparator << " " << i << " " << best << " " << size << endl;
            if (comparator(&shapes[i], best) < 0) {
//cout << __LINE__ << endl;
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

    void move(Shape &shape) {
        total += shape.score();
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
            cout << (height - 1 - moves[i].y) << " " << moves[i].x << endl;
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

int fromLeft(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    return a->minX - b->minX;
}

int fromSmallestWithoutOne(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    int ca = a->c == 1;
    int cb = b->c == 1;
    if (ca != cb) {
        return ca - cb;
    }
    return a->size - b->size;
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

int randomStrategy(const Shape *a, const Shape *b) {
    if (a->vs != b->vs) {
        return a->vs - b->vs;
    }
    return a->rand - b->rand;
}


void validate(Board *board, ShapeList& list) {
    int total = 0;
    for (int i = 0; i < list.size; i++) {
        total += list.shapes[i].size;
        int x = list.shapes[i].minX;
        int y = list.shapes[i].y;
        if (board->board[x][y] != list.shapes[i].c || !list.shapes[i].c) {
            cout << "B " << char('A' + board->board[x][y]) << char('A' + list.shapes[i].c) << endl;
            list.shapes[i].print();
        }
        if (list.shapes[i].y < list.shapes[i].minY || list.shapes[i].y > list.shapes[i].maxY) {
            cout << "D " << endl;
            list.shapes[i].print();
        }
        int sum =
            (x > 0 && board->board[x - 1][y] == list.shapes[i].c) +
            (x < board->w - 1 && board->board[x + 1][y] == list.shapes[i].c) +
            (y > 0 && board->board[x][y - 1] == list.shapes[i].c) +
            (y < board->h - 1 && board->board[x][y + 1] == list.shapes[i].c);
        if (!sum) {
            cout << "C" << endl;
        }
    }
    int total2 = 0;
    for (int x = 0; x < board->w; x++) {
        for (int y = 0; y < board->h; y++) {
            int sum =
                (x > 0 && board->board[x - 1][y] == board->board[x][y]) +
                (x < board->w - 1 && board->board[x + 1][y] == board->board[x][y]) +
                (y > 0 && board->board[x][y - 1] == board->board[x][y]) +
                (y < board->h - 1 && board->board[x][y + 1] == board->board[x][y]);
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
    }
}

void calcHistogram(Board *board) {
    memset(colorHistogram, 0, sizeof(colorHistogram));

    for (int x = 0; x < board->w; x++) {
        char* col = board->board[x];
        for (int y = 0; y < board->h; y++) {
            colorHistogram[*col]++;
            col++;
        }
    }

    char mostPopularColor = -1;
    int mostPopularCount = 0;
    for (int c = 0; c <= MAX_COLOR; c++) {
        if (colorHistogram[c] > mostPopularCount) {
            mostPopularColor = c;
            mostPopularCount = colorHistogram[c];
        }
    }
}

void test(Board *board, int (*comparator)(const Shape*, const Shape*), Game &game) {
    //board->print();

    ShapeList list;
    /*for (int i = 0; i < 1000000; i++) {
        list.update(board);
    }*/
    list.update(board);
    //validate(board, list);
    //list.print();

    //cout << endl;
    /*ShapeList list2;
    list2.update(board, 2, 3);
    list2.update(board, 6, 6);
    list2.update(board, 7, 7);
    list2.update(board, 0, 0);
    list2.update(board, 3, 5);
    list2.update(board, 1, 1);
    list2.update(board, 4, 5);
    list2.print();*/

    game.total = 0;
    game.nmoves = 0;
//cout << __LINE__ << endl;
    while(list.size) {
        //list.sort(comparator);
//cout << __LINE__ << endl;
        Shape& move = list.findBest(comparator);
//cout << __LINE__ << endl;
        game.move(move);
//cout << __LINE__ << endl;
        board->remove(move);
//cout << __LINE__ << endl;
//cout << __LINE__ << " " << move.minX << " " << move.maxX << " " << move.minY << endl;
        list.update(board, move.minX - 1, move.maxX + 1, move.minY - 1);
//cout << __LINE__ << endl;
        //board->print();
        //list.print();
        //validate(board, list);
    }
//cout << __LINE__ << endl;
}

/*const int NCOMP = 12;
int (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromSmallest, fromLargest, fromBottom, fromTop, fromLeft, fromSmallestWithoutOne, byColorAndFromSmallest,
    byColorAndFromLargest, byColorAndFromTop, byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop
};*/
const int NCOMP = 10;
int (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromLargest, fromTop, fromSmallestWithoutOne, fromLargestWithoutMostPop, byColorAndFromSmallest,
    byColorAndFromLargest, byColorAndFromTop, byColorNoAndFromSmallest, byColorNoAndFromLargest, byColorNoAndFromTop
};
/*const int NCOMP = 3;
int (*comparators[NCOMP])(const Shape*, const Shape*) = {
    fromTop, byColorAndFromLargest, byColorAndFromTop
};*/

Game games[NCOMP];
int hist[NCOMP] = {0};
Game* test2(Board *board) {
    calcHistogram(board);
    int bestGame;
    long bestScore = -1;
    for (int i = 0; i < NCOMP; i++) {
        Board board2 = *board;
        test(&board2, comparators[i], games[i]);
        if (games[i].total > bestScore) {
            bestGame = i;
            bestScore = games[i].total;
        }
    }
    hist[bestGame]++;
    return games + bestGame;
}

Move pickRandomMove(Board *board) {
    Move move;
    for (int i = 0; i < board->w * board->h; i++) {
        int x = random() % board->w;
        int h = board->size[x];
        if (h == 0) {
            continue;
        }
        int y = random() % h;
        char c = board->board[x][y];
        if (!c) {
            board->size[x] = y;
            continue;
        }
        if ((x > 0 && board->board[x - 1][y] == c) ||
            (x < board->w - 1 && board->board[x + 1][y] == c) ||
            (y > 0 && board->board[x][y - 1] == c) ||
            (y < board->h - 1 && board->board[x][y + 1] == c)) {
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
    for (int x = 0; x < board->w; x++) {
        int h = board->size[x];
        for (int y = h - 1; y >= 0; y--) {
            char c = board->board[x][y];
            if (!c) {
                board->size[x] = y;
                continue;
            }
            if ((x > 0 && board->board[x - 1][y] == c) ||
                (x < board->w - 1 && board->board[x + 1][y] == c) ||
                (y > 0 && board->board[x][y - 1] == c) ||
                (y < board->h - 1 && board->board[x][y + 1] == c)) {
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
    game.total = 0;
    game.nmoves = 0;
    for (int x = 0; x < board->w; x++) {
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

void stats() {
    //int width = 20;
    //int height = 50;
    long total2 = 0, total3 = 0;
    for (int ncols = 4; ncols <= 20; ncols += 2) {
        long total[NCOMP + 2] = {0};
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
            total2 += game->total * ncols * ncols / width / height;
            board2 = *board;
            game = randomPlayer2(&board2);
            total[NCOMP + 1] += game->total;
            total3 += game->total * ncols * ncols / width / height;
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
        cout << ncols << " " << total[NCOMP] << " " << total[NCOMP + 1] << /*" " << total[NCOMP] * ncols * ncols / width / height <<*/ endl;
        for (int i = 0; i < NCOMP; i++) {
            cout << ncols << " " << i << " " << hist[i] << endl;
        }
    }
    cout << total2 << " " << total3 << endl;
}

int main() {
    //stats();
    //play();
    randomPlay();
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
