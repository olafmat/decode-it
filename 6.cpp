#include <algorithm>
#include <stack>
#include <vector>
#include <iostream>
#include <iomanip>
#include <memory.h>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;

typedef double real;

const real PI2 = (real)M_PI / 2;

struct Coordinates {
    real x, y;
};

struct Circle: public Coordinates {
    real r;
};

struct Point: public Coordinates  {
    Circle *circle;
    real angle;
};

real dist2(const Coordinates* a, const Coordinates* b) {
    real dx = a -> x - b -> x;
    real dy = a -> y - b -> y;
    return dx * dx + dy * dy;
}

real direction(const Coordinates* z, const Coordinates* a, const Coordinates* b) {
    return (a -> x - z -> x) * (b -> y - z -> y) - (a -> y - z -> y) * (b -> x - z -> x);
}

bool comparator(Point* a, Point* b) {
    return a->x < b->x || (a->x == b->x && a->y < b->y) || (a->x == b->x && a->y == b->y && (long)(void*)a < (long)(void*)b);
}

vector<Point*> findConvexHull(vector<Point*> &points) {
    int n = points.size();
    if (n <= 3) {
        return points;
    }

    sort(points.begin(), points.end(), comparator);

    vector<Point*> hull(n << 1);

    int k = 0;
    for (int i = 0; i < n; i++) {
        while (k >= 2 && direction(hull[k - 2], hull[k - 1], points[i]) <= 0) {
            k--;
        }
        hull[k++] = points[i];
    }
    
    for (int i = n - 1, t = k + 1; i > 0; i--) {
        while (k >= t && direction(hull[k - 2], hull[k - 1], points[i - 1]) <= 0) {
            k--;
        }
        hull[k++] = points[i - 1];
    }

    hull.resize(k - 1);
    return hull;
}

void removeInternal(vector<Circle*> &out, vector<Circle*> &in) {
    for (vector<Circle*>::iterator i = in.begin(); i != in.end(); i++) {
        const real r1 = (*i)->r;

        bool enclosed = false;
        bool wasI = false;
        for (vector<Circle*>::iterator j = in.begin(); j != in.end(); j++) {
            if (*i == *j) {
                wasI = true;
                continue;
            }
            const real r2 = (*j)->r;
            real d2 = dist2(*i, *j);
            if (d2 < 0) {
                d2 = 0;
            }
            const real d = sqrt(d2) + r1 - r2;
            if (d < 0 || (d <= 0 && wasI)) {
                enclosed = true;
                break;
            }
        }

        if (!enclosed) {
            out.push_back(*i);
        }
    }
}

void findEdge(vector<Point*> &points, vector<Circle*> &circles) {
    int size = circles.size();
    int pcount = 225000 / size;
    if (pcount < 16) {
        pcount = 16;
    }
    if (pcount > 400) {
        pcount = 400;
    }

    real total = 0;
    for (vector<Circle*>::iterator i = circles.begin(); i != circles.end(); i++) {
        total += (*i)->r;
    }
    real step = total / 225000;

    for (vector<Circle*>::iterator i = circles.begin(); i != circles.end(); i++) {
        const real x1 = (*i)->x;
        const real y1 = (*i)->y;
        const real r1 = (*i)->r;
        int pcount2 = min(pcount, int(r1 / step + 0.5));
        if (pcount2 < 1) {
            pcount2 = 1;
        }
        for (int a = 0; a < pcount2; a++) {
            const real angle = (real) M_PI * a * 2 / pcount2;
            Point* p1 = new Point();
            p1 -> x = x1 + r1 * sin(angle);
            p1 -> y = y1 + r1 * cos(angle);
            p1 -> circle = *i;
            p1 -> angle = angle;
            points.push_back(p1);
        }
    }
}

void findTangle(vector<Point*> &points, Circle *a, Circle *b) {
    const real x1 = a->x;
    const real y1 = a->y;
    const real r1 = a->r;
    const real x2 = b->x;
    const real y2 = b->y;
    const real r2 = b->r;
    const real dx = x2 - x1;
    const real dy = y2 - y1;
    const real dr = r2 - r1;
    const real d2 = dx * dx + dy * dy;
    const real d = d2 > 0 ? sqrt(d2) : 0.0;
    const real beta1 = dr > d ? PI2 : dr < -d ? -PI2 : asin(dr / d);
    const real gamma = -atan2(dy, dx);
    for (int sign = -1; sign <= 1; sign += 2) {
        const real beta = beta1 * sign;
        const real alpha = gamma - beta;
        const real sinAlpha = sin(alpha) * sign;
        const real cosAlpha = cos(alpha) * sign;

        Point* p1 = new Point();
        p1 -> x = x1 + r1 * sinAlpha;
        p1 -> y = y1 + r1 * cosAlpha;
        p1 -> circle = a;
        p1 -> angle = fmod(sign > 0 ? alpha + (real)M_PI * 2: alpha + (real)M_PI * 3, (real)M_PI * 2);
        points.push_back(p1);

        Point* p2 = new Point();
        p2 -> x = x2 + r2 * sinAlpha;
        p2 -> y = y2 + r2 * cosAlpha;
        p2 -> circle = b;
        p2 -> angle = fmod(sign > 0 ? alpha + (real)M_PI * 2: alpha + (real)M_PI * 3, (real)M_PI * 2);
        points.push_back(p2);
    }
}

Point* findNearest(vector<Point*> &points, const Point *a) {
    Point* best = points[0];
    real dist = dist2(best, a);
    for (int i = 1; i < points.size(); i++) {
        real d = dist2(points[i], a);
        if (d < dist) {
            best = points[i];
            dist = d;
        }
    }
    return best;
}

real beltLength(const Point *a, const Point *b) {
    if (a == b) {
        return 0;
    } else if (a -> circle != b -> circle) {
        real d2 = dist2(a, b);
        return d2 > 0 ? sqrt(d2) : 0.0;
    } else if (a -> angle >= b -> angle) {
        return a -> circle -> r * (M_PI * 2 + b -> angle - a -> angle);
    } else {
        return a -> circle -> r * (b -> angle - a -> angle);
    }
}

real beltLength(vector<Point*> &points) {
    if (points.empty()) {
        return 0;
    }

    Point* prev = points[points.size() - 1];
    vector<Point*> points2;
    for (vector<Point*>::iterator it = points.begin(); it != points.end(); it++) {
        Point* p = *it;
        if (p->circle != prev->circle) {
            vector<Point*> tangle;
            findTangle(tangle, p->circle, prev->circle);
            points2.push_back(findNearest(tangle, prev));
            points2.push_back(findNearest(tangle, p));
        }
        prev = p;
    }

    real length = 0;
    prev = points2[points2.size() - 1];
    for (vector<Point*>::iterator it = points2.begin(); it != points2.end(); it++) {
        Point* p = *it;
        real len = beltLength(p, prev);
        length += len;
        prev = p;
    }
    return length;
}

int main() {
    int t;
    cin >> t;

    for (int i = 0; i < t; i++) {
        int n;
        cin >> n;
        vector<Circle*> circles;

        for (int j = 0; j < n; j++) {
            Circle* c = new Circle();
            cin >> c -> x >> c -> y >> c -> r;
            c -> r = fabs(c -> r);
            circles.push_back(c);
        }

        vector<Circle*> circles2;
        removeInternal(circles2, circles);

        if (circles2.size() == 1) {
            cout << fixed << setprecision(10) << circles2[0]->r * M_PI * 2 << endl;
            continue;
        }

        vector<Point*> points;
        findEdge(points, circles2);

        vector<Point*> result = findConvexHull(points);

        cout << fixed << setprecision(10) << beltLength(result) << endl;
    }

    return 0;
}
