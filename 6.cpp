#include <algorithm>
#include <stack>
#include <vector>
#include <iostream>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;

typedef double real;

const real epsilon = 0;//(real)1e-15;
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

    /*for (int i = 0; i < n; i++) {
        Point *p = points[i];
        cout << p->x << " " << p->y << " " << p->circle << " " << p->angle * 180 / M_PI << endl;
    }*/

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

    /*for (int i = n - 1; i > 0; i--) {
        while (k >= 2 && direction(hull[k - 2], hull[k - 1], points[i - 1]) <= 0) {
            k--;
        }
        hull[k++] = points[i - 1];
    }*/

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
            if (d < -epsilon || (d <= epsilon && wasI)) {
                enclosed = true;
                break;
            }
        }

        if (!enclosed) {
            out.push_back(*i);
        }
    }
}

void outerTangle(vector<Point*> &points, vector<Circle*> &circles) {
    int size = circles.size();
    int pcount = 225000 / size;
    if (pcount < 16) {
        pcount = 16;
    }
    if (pcount > 400) {
        pcount = 400;
    }
    //160000 16 500 891.9 6.26 2.9  8.37
    //160000 16 600 891.9 6.52
    //170000 16 500 789.3      3.01 8.37
    //160000 17 500 891.9 6.23 2.84 8.37
    //160000 16 400 891.9 5.73 2.74 8.37
    //long double too long
    //float         687.6 4.91 2.39 7.83
    //circ+tangents too long
    //220000 16 400 892.8 5.75 2.69 8.46
    //250000 16 400 891.9 6.05 2.91 8.28
    //220000 250000 891   5.92 2.83 8.28
    //250000 220000 891   6.42 2.79 8.28
    //230000 230000 891   5.97 2.79 8.28
    //210000 210000 888.3 5.72 2.65 8.1
    //225000 225000 895.5 5.84 2.74 8.64
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
        if (pcount2 < 4) {
            pcount2 = 4;
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
        /*for (vector<Circle*>::iterator j = i + 1; j != circles.end(); j++) {
            const real x2 = (*j)->x;
            const real y2 = (*j)->y;
            const real r2 = (*j)->r;
            const real dx = x2 - x1;
            const real dy = y2 - y1;
            const real dr = r2 - r1;
            const real d2 = dx * dx + dy * dy;
            const real d = d2 > 0 ? sqrt(d2) : 0.0;
            if (d < epsilon || abs(dr) > d + epsilon) {
                continue;
            }
            const real beta1 = dr > d - epsilon ? PI2 : dr < -d + epsilon ? -PI2 : asin(dr / d);
            const real gamma = -atan2(dy, dx);
            for (int sign = -1; sign <= 1; sign += 2) {
                const real beta = beta1 * sign;
                const real alpha = gamma - beta;
                const real sinAlpha = sin(alpha) * sign;
                const real cosAlpha = cos(alpha) * sign;

                Point* p1 = new Point();
                p1 -> x = x1 + r1 * sinAlpha;
                p1 -> y = y1 + r1 * cosAlpha;
                p1 -> circle = *i;
                p1 -> angle = fmod(sign > 0 ? alpha + (real)M_PI * 2: alpha + (real)M_PI * 3, (real)M_PI * 2);
                points.push_back(p1);

                Point* p2 = new Point();
                p2 -> x = x2 + r2 * sinAlpha;
                p2 -> y = y2 + r2 * cosAlpha;
                p2 -> circle = *j;
                p2 -> angle = fmod(sign > 0 ? alpha + (real)M_PI * 2: alpha + (real)M_PI * 3, (real)M_PI * 2);
                points.push_back(p2);
            }
        }*/
    }
}

real beltLength(const Point *a, const Point *b) {
    if (a -> circle != b -> circle) {
        real d2 = dist2(a, b);
        return d2 > 0 ? sqrt(d2) : 0.0;
    } else if (a -> angle >= b -> angle - epsilon) {
        return a -> circle -> r * (M_PI * 2 + b -> angle - a -> angle);
    } else {
        return a -> circle -> r * (b -> angle - a -> angle);
    }
}

real beltLength(vector<Point*> &points) {
    if (points.empty()) {
        return 0;
    }
    real length = 0;
    const Point* prev = points[points.size() - 1];
    vector<Point*>::iterator it;
    for (it = points.begin(); it != points.end(); it++) {
        const Point* p = *it;
        real len = beltLength(p, prev);
        //cout << p->x << " " << p->y << " " << len << " " << p->circle << " " << p->angle * 180 / M_PI << endl;
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
        outerTangle(points, circles2);

        vector<Point*> result = findConvexHull(points);

        cout << fixed << setprecision(10) << beltLength(result) << endl;
    }

    return 0;
}
