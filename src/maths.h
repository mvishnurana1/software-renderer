#if !defined(MATHS_H)
#define MATHS_H

struct v2{
    union{
        struct{
            float x,y;
        };
        float e[2];
    };

    float & operator [] (const int idx);

    const float & operator [] (const int idx) const;

    v2 operator - (const v2 & rhs) const;

    v2 operator - (const float rhs) const;

    v2 operator / (const float rhs) const;

    v2 operator + (const v2 & rhs);

    v2 operator + (const float rhs);

    float operator * (const v2 & rhs) const;

    v2 operator * (const float rhs) const;

    float inner(const v2 & rhs);

    inline float length_sq();

    inline float length();

    v2 normalise();
};

struct v3{
    union{
        struct{
            float x,y,z;
        };
        float e[3];
    };

    float & operator [] (const int idx);

    const float & operator [] (const int idx) const;

    v3 operator - (const v3 & rhs) const;

    v3 operator - (const float rhs) const;

    v3 operator / (const float rhs) const;

    v3 operator + (const v3 & rhs);

    v3 operator + (const float rhs);

    float operator * (const v3 & rhs) const;

    v3 operator * (const float rhs) const;

    float inner(const v3 & rhs);

    inline float length_sq();

    inline float length();

    v3 normalise();
};

struct v4{
    union{
        struct{
            float x,y,z,w;
        };
        float e[4];
    };

    float & operator [] (const int idx);

    const float & operator [] (const int idx) const;

    v4 operator - (const v4 & rhs) const;

    v4 operator - (const float rhs) const;

    v4 operator / (const float rhs) const;

    v4 operator + (const v4 & rhs);

    v4 operator + (const float rhs);

    float operator * (const v4 & rhs) const;

    v4 operator * (const float rhs) const;

    float inner(const v4 & rhs);

    inline float length_sq();

    inline float length();

    v4 normalise();
};

inline v3 cross(const v3 &v1, const v3 &v2);
inline v4 project_4d(const v3& v, float end = 1);
inline v3 project_3d(const v4& v);

struct v2_i {
    union
    {
        struct
        {
            int x, y;
        };
        int e[3];
    };
};

v2_i v3_to_v2(const v3& v);
v2_i operator + (const v2_i& lhs, const v2_i& rhs);
v2_i operator - (const v2_i& lhs, const v2_i& rhs);

struct v3_i {
    union
    {
        struct
        {
            int x, y, z;
        };
        int e[3];
    };
};

struct m2{
    union{
        struct{
            v2 r1, r2;
        };
        v2 e[2];
    };

    inline v2 & operator[](const int idx);
    
    inline const v2 & operator[](const int idx) const;
    
    inline m2 operator * (const m2 & rhs) const; 
    
    inline v2 operator *(const v2 & rhs) const;
    
    inline m2 operator * (const float rhs) const;

        

    inline float determinate() const;
    
    
    
    

    inline m2  transpose() const;

    inline float sum() const;
};

struct m3{
    union{
        struct{
            v3 r1, r2, r3;
        };
        v3 e[3];
    };

    inline v3 & operator[](const int idx);
    
    inline const v3 & operator[](const int idx) const;
    
    inline m3 operator * (const m3 & rhs) const; 
    
    inline v3 operator *(const v3 & rhs) const;
    
    inline m3 operator * (const float rhs) const;

    inline m2 get_sub_mat(int row_to_skip, int col_to_skip) const;    

    inline float determinate() const;
    
    inline m3 adjugate() const;
    
    inline m3 invert() const;

    inline m3  transpose() const;

    inline float sum() const;
};

struct m4{
    union{
        struct{
            v4 r1, r2, r3, r4;
        };
        v4 e[4];
    };

    inline v4 & operator[](const int idx);
    
    inline const v4 & operator[](const int idx) const;
    
    inline m4 operator * (const m4 & rhs) const; 
    
    inline v4 operator *(const v4 & rhs) const;
    
    inline m4 operator * (const float rhs) const;

    inline m3 get_sub_mat(int row_to_skip, int col_to_skip) const;    

    inline float determinate() const;
    
    inline m4 adjugate() const;
    
    inline m4 invert() const;

    inline m4  transpose() const;

    inline float sum() const;
};

m4 look_at(const v3 & eye, const v3 & center, const v3 & up);
m4 identity();
m4 view_port(float x, float y, float width, float height);
m4 projection(const v3 & eye, const v3 & center);
m3 m4_to_m3(const m4 & rhs);
m4 rot_x(float deg);
m4 rot_y(float deg);
m4 trans(const v3& vec);
m4 scale(const v3& vec);

inline int sign(float num);
#endif