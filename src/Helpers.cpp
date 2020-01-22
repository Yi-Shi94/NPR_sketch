#include "Helpers.h"

#include <iostream>
#include <fstream>
using namespace std;
using namespace Eigen;

void VertexArrayObject::init()
{
  glGenVertexArrays(1, &id);
  check_gl_error();
}

void VertexArrayObject::bind()
{
  glBindVertexArray(id);
  check_gl_error();
}

void VertexArrayObject::free()
{
  glDeleteVertexArrays(1, &id);
  check_gl_error();
}

void VertexBufferObject::init()
{
  glGenBuffers(1,&id);
  check_gl_error();
}

void VertexBufferObject::bind()
{
  glBindBuffer(GL_ARRAY_BUFFER,id);
  check_gl_error();
}

void VertexBufferObject::free()
{
  glDeleteBuffers(1,&id);
  check_gl_error();
}

void VertexBufferObject::update(const Eigen::MatrixXf& M)
{
  assert(id != 0);
  glBindBuffer(GL_ARRAY_BUFFER, id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*M.size(), M.data(), GL_DYNAMIC_DRAW);
  rows = M.rows();
  cols = M.cols();
  check_gl_error();
}

bool Program::init(
  const std::string &vertex_shader_string,
  const std::string &fragment_shader_string,
  const std::string &fragment_data_name)
{
  using namespace std;
  vertex_shader = create_shader_helper(GL_VERTEX_SHADER, vertex_shader_string);
  fragment_shader = create_shader_helper(GL_FRAGMENT_SHADER, fragment_shader_string);

  if (!vertex_shader || !fragment_shader)
    return false;

  program_shader = glCreateProgram();

  glAttachShader(program_shader, vertex_shader);
  glAttachShader(program_shader, fragment_shader);

  glBindFragDataLocation(program_shader, 0, fragment_data_name.c_str());
  glLinkProgram(program_shader);

  GLint status;
  glGetProgramiv(program_shader, GL_LINK_STATUS, &status);

  if (status != GL_TRUE)
  {
    char buffer[512];
    glGetProgramInfoLog(program_shader, 512, NULL, buffer);
    cerr << "Linker error: " << endl << buffer << endl;
    program_shader = 0;
    return false;
  }

  check_gl_error();
  return true;
}

void Program::bind()
{
  glUseProgram(program_shader);
  check_gl_error();
}

GLint Program::attrib(const std::string &name) const
{
  return glGetAttribLocation(program_shader, name.c_str());
}

GLint Program::uniform(const std::string &name) const
{
  return glGetUniformLocation(program_shader, name.c_str());
}

GLint Program::bindVertexAttribArray(
        const std::string &name, VertexBufferObject& VBO) const
{
  GLint id = attrib(name);
  if (id < 0)
    return id;
  if (VBO.id == 0)
  {
    glDisableVertexAttribArray(id);
    return id;
  }
  VBO.bind();
  glEnableVertexAttribArray(id);
  glVertexAttribPointer(id, VBO.rows, GL_FLOAT, GL_FALSE, 0, 0);
  check_gl_error();

  return id;
}

void Program::free()
{
  if (program_shader)
  {
    glDeleteProgram(program_shader);
    program_shader = 0;
  }
  if (vertex_shader)
  {
    glDeleteShader(vertex_shader);
    vertex_shader = 0;
  }
  if (fragment_shader)
  {
    glDeleteShader(fragment_shader);
    fragment_shader = 0;
  }
  check_gl_error();
}

GLuint Program::create_shader_helper(GLint type, const std::string &shader_string)
{
  using namespace std;
  if (shader_string.empty())
    return (GLuint) 0;

  GLuint id = glCreateShader(type);
  const char *shader_string_const = shader_string.c_str();
  glShaderSource(id, 1, &shader_string_const, NULL);
  glCompileShader(id);

  GLint status;
  glGetShaderiv(id, GL_COMPILE_STATUS, &status);

  if (status != GL_TRUE)
  {
    char buffer[512];
    if (type == GL_VERTEX_SHADER)
      cerr << "Vertex shader:" << endl;
    else if (type == GL_FRAGMENT_SHADER)
      cerr << "Fragment shader:" << endl;
    else if (type == GL_GEOMETRY_SHADER)
      cerr << "Geometry shader:" << endl;
    cerr << shader_string << endl << endl;
    glGetShaderInfoLog(id, 512, NULL, buffer);
    cerr << "Error: " << endl << buffer << endl;
    return (GLuint) 0;
  }
  check_gl_error();

  return id;
}

void _check_gl_error(const char *file, int line)
{
  GLenum err (glGetError());
  while(err!=GL_NO_ERROR)
  {
    std::string error;

    switch(err)
    {
      case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
      case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
      case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
      case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:  error="INVALID_FRAMEBUFFER_OPERATION";  break;
    }
    std::cerr << "GL_" << error.c_str() << " - " << file << ":" << line << std::endl;
    err = glGetError();
  }
}

void split(const string& s,
           vector<string>& sv,
           const char delim = '.') {
    sv.clear();
    istringstream iss(s);
    string temp;
    
    while (getline(iss, temp, delim)) {
        sv.emplace_back(move(temp));
    }
    return;
}


TriMesh::TriMesh(string name,int start,double degree){
    if (name=="ini_placeholder") return;
    vector<string> name_token;
    split(name,name_token,'.');
    if(name_token[1]=="off"){
        //cout<<"off detected"<<endl;
        this->readoff(name);
    } else {
        //cout<<"obj detected"<<endl;
        this->readobj(name);
    }
    
    Vector3d ini_color(0.1,0.1,0.1);
    this->set_color(ini_color);
    this->compute_bary_n_max();
    //this->set_trans_mat(-this->barycenter(0),-this->barycenter(1),-this->barycenter(2),0,degree,0,1);
    this->start = start;
    cal_normal_matrix();
}

void TriMesh::readoff(string fname){
    ifstream infile;
    infile.open(fname);
    if (!infile) cerr << "Could not open the file!" << endl;
    string line_str;
    int index_line = 0;
    
    vector<Vector3d> vertices;
    vector<Vector3d> faces;
    vector<Vector3d> face_index;
    while (getline(infile, line_str))
    {
        if (index_line==0) {
            if(line_str!="OFF"){
                cerr<<"Only off file allowed!"<<endl;
            }
        } else {
            istringstream iss(line_str);
            
            if (index_line==1) {
                int a1,a2,a3;
                if (!(iss >> a1 >> a2 >> a3)) {
                    cerr<<"off corruted!"<<endl;
                    break;
                } else {
                    //cout<<"num:"<<a1<<' '<<a2<<' '<<endl;
                }
            } else {
                vector<string> data_vec;
                string tmp;
                while (iss >> tmp) {
                    data_vec.push_back(tmp);
                }
                if (data_vec.size()==3) {
                    Vector3d vertex(stod(data_vec[0]),stod(data_vec[1]),stod(data_vec[2]));
                    vertices.push_back(vertex);
                } else if (data_vec.size()==4){
                    Vector3d plain(stoi(data_vec[1]),stoi(data_vec[2]),stoi(data_vec[3]));
                    //cout<<data_vec[1]<<' '<<data_vec[2]<<' '<<data_vec[3];
                    
                    face_index.push_back(plain);
                    Vector3d a = vertices[stoi(data_vec[1])];
                    Vector3d b = vertices[stoi(data_vec[2])];
                    Vector3d c = vertices[stoi(data_vec[3])];
                    faces.push_back(a);
                    faces.push_back(b);
                    faces.push_back(c);
                } else{
                    cerr<<"double/int size:"<<data_vec.size()<<endl;
                }
            }
        }
        index_line ++;
    }
    
    //cout<<"loading success! "<<fname<<' '<<vertices.size()<<' '<<face_index.size()<<' '<<faces.size()<<endl;
    this->faces = faces;
    this->face_index = face_index;
    this->vertices = vertices;
    this->tri_num = this->face_index.size();
    //this->compute_bary_n_max();
    //this->set_trans_mat(this->barycenter(0),
     //                   this->barycenter(1),
      //                  this->barycenter(2),
      //                    0,0,0,1);
}

void TriMesh::readobj(string fname){
    
    vector<Vector3d> vertices;
    vector<Vector3d> faces;
    vector<Vector3d> face_index;
    
    
    ifstream fin(fname);
    int index_line = 0;
    
    if(!fin)
    {
        cerr << "Could not open the file!" << endl;
        return;
    }
    string line_str;
    while(getline(fin, line_str))
    {
        //cout<<line_str<<endl;
        vector<string> num_token;
        if(line_str[0]=='v'&&line_str[1]==' '){
            
            split(line_str,num_token,' ');
            //cout<<"fuck"<<fin<<endl;
           // cout<<index_line<<" :v: "<<num_token[1]<<' '<<num_token[2]<<' '<<num_token[3]<<endl;
            Vector3d vertex(stod(num_token[1]),stod(num_token[2]),stod(num_token[3]));
            vertices.push_back(vertex);
            
        } else if(line_str[0]=='f'&&line_str[1]==' '){
            
           // cout<<line_str<<endl;
            split(line_str,num_token,' ');
            vector<string> subtoken_1,subtoken_2,subtoken_3;
            //cout<<num_token[2]<<' '<<num_token[3]<<' '<<num_token[4]<<endl;
            //cout<<vertices.size()<<endl;
            split(num_token[2],subtoken_1,'/');
            split(num_token[3],subtoken_2,'/');
            split(num_token[4],subtoken_3,'/');
            int f_x = stoi(subtoken_1[0]);
            int f_y = stoi(subtoken_2[0]);
            int f_z = stoi(subtoken_3[0]);
        //
           // cout<<index_line<<" :f: "<<f_x<<' '<<f_y<<' '<<f_z<<endl;
            Vector3d plain(f_x,f_y,f_z);
            face_index.push_back(plain);
            Vector3d a = vertices[f_x-1];
            Vector3d b = vertices[f_y-1];
            Vector3d c = vertices[f_z-1];
            faces.push_back(a);
            faces.push_back(b);
            faces.push_back(c);
        
        }
        index_line ++;
    }
   
    //cout<<"loading success! "<<fname<<' '<<vertices.size()<<' '<<face_index.size()<<' '<<faces.size()<<endl;
    this->faces = faces;
    this->face_index = face_index;
    this->vertices = vertices;
    this->tri_num = this->face_index.size();
//this->compute_bary_n_max();
    
}

void TriMesh::compute_bary_n_max(){
     vector<Vector3d> vertices = this->vertices;
     //double x=0,y=0,z=0;
     double x_max = -10000, x_min = 10000, y_max = -10000, y_min = 10000 ,z_max = -10000,z_min  =10000;
     for(int i=0;i<vertices.size();i++){
         if (vertices[i](0)>x_max) x_max = vertices[i](0);
         else if(vertices[i](0)<x_min) x_min = vertices[i](0);
         if (vertices[i](1)>y_max) y_max = vertices[i](1);
         else if(vertices[i](1)<y_min) y_min = vertices[i](1);
         if (vertices[i](2)>z_max) z_max = vertices[i](2);
         else if(vertices[i](2)<z_min) z_min = vertices[i](2);
     }
     //cout<<x_max<<" x "<<x_min<<' '<<x_max-x_min <<endl;
     //cout<<y_max<<" y "<<y_min<<' '<<y_max-y_min <<endl;
     //cout<<z_max<<" z "<<z_min<<' '<<z_max-z_min <<endl;
     x=(x_max+x_min)/2.0;
     y=(y_max+y_min)/2.0;
     z=(z_max+z_min)/2.0;
     //cout<<"center: "<<x<<' '<<y<<' '<<z<<endl;
     Vector3d res(x,y,z);
     this->barycenter=res;
     //cout<<res<<endl;
}

/*
void TriMesh::compute_stdv(){
    double variance_x = 0;
    double variance_y = 0;
    double
    for(int i=0;i<vertices.size();i++){
        vertices[i](0) -
    }
    
}
*/


MatrixXf TriMesh::get_matrix(){
    vector<Vector3d> vertices = this->faces;
    MatrixXf v_matrix(3,vertices.size());
    for(int i=0;i<vertices.size();i++){
        v_matrix.col(i)<<vertices[i](0),vertices[i](1),vertices[i](2);
    }
    return v_matrix;
}


MatrixXf TriMesh::get_normal_matrix()
{
    MatrixXf v_matrix(3,tri_num*3);
    for(int i=0;i<normals.size();i=i+3){
        Vector3d normal = normals[i];
        v_matrix.col(i)  <<normal(0),normal(1),normal(2);
        v_matrix.col(i+1)<<normal(0),normal(1),normal(2);
        v_matrix.col(i+2)<<normal(0),normal(1),normal(2);
    }
    return v_matrix;
}

void TriMesh::cal_normal_matrix(){
    vector<Vector3d> vertices = this->faces;
    vector<Vector3d> normal_per_ver;
    Matrix4f trans = get_trans_mat();
    //cout<<tri_num<<' '<<vertices.size()<<endl;
    for(int i=0;i<vertices.size();i=i+3){
        MatrixXf points(4,3);
        points.col(0) << vertices[i](0),vertices[i](1),vertices[i](2),1;
        points.col(1) << vertices[i+1](0),vertices[i+1](1),vertices[i+1](2),1;
        points.col(2) << vertices[i+2](0),vertices[i+2](1),vertices[i+2](2),1;
        points = trans*points;
        
        Vector3d x1(points(0,0),points(1,0),points(2,0));
        Vector3d x2(points(0,1),points(1,1),points(2,1));
        Vector3d x3(points(0,2),points(1,2),points(2,2));
        
        Vector3d a = x2 - x1;
        Vector3d b = x3 - x1;
        Vector3d normal = a.cross(b).normalized();
        normal_per_ver.push_back(normal);
        normal_per_ver.push_back(normal);
        normal_per_ver.push_back(normal);
    }
    normals = normal_per_ver;
}

MatrixXf TriMesh::get_trans_mat(){
    
    MatrixXf rot_x(4,4);
    MatrixXf rot_y(4,4);
    MatrixXf rot_z(4,4);
    MatrixXf scal(4,4);
    MatrixXf trans(4,4);
    //MatrixXf trans_final(4,4);
    
    float rad_x = angle_x*3.1415926535/180.;
    float rad_y = angle_y*3.1415926535/180.;
    float rad_z = angle_z*3.1415926535/180.;
    
    float cos_x = cos(rad_x);
    float cos_y = cos(rad_y);
    float cos_z = cos(rad_z);
    
    float sin_x = sin(rad_x);
    float sin_y = sin(rad_y);
    float sin_z = sin(rad_z);
    float s = scale;
    scal<<s,0,0,0,
          0,s,0,0,
          0,0,s,0,
          0,0,0,1;
    
    trans<< 1,0,0,x,
            0,1,0,y,
            0,0,1,z,
            0,0,0,1;
    
    rot_x<<  1,0,0,0,
            0,cos_x,-sin_x,0,
            0,sin_x,cos_x,0,
            0,0,0,1;
    
    rot_y<<cos_y,    0,     sin_y,       0,
           0,        1,     0,        0,
          -sin_y,     0,     cos_y, 0,
           0,        0,     0,        1.;
    
    rot_z<<cos_z,-sin_z,0,0,
          sin_z,cos_z,  0,0,
          0,    0,      1,0,
          0,    0,      0,1;
    
    //trans_final<< 1,0,0,barycenter(0),
    //            0,1,0,barycenter(1),
     //           0,0,1,barycenter(2),
     //           0,0,0,1;
    
    //compute_bary_n_max(type);
    
    return trans*rot_z*rot_y*rot_x*scal;
}

void TriMesh::set_trans_mat(float x,float y, float z, float angle_x, float angle_y, float angle_z,float s){
    this->x = x;
    this->y = y;
    this->z = z;
    this->angle_x = angle_x;
    this->angle_y = angle_y;
    this->angle_z = angle_z;
    this->scale = s;
}

void TriMesh::alter_trans_mat(float x,float y, float z, float angle_x, float angle_y, float angle_z,float s){
    this->x += x;
    this->y += y;
    this->z += z;
    this->angle_x += angle_x;
    this->angle_y += angle_y;
    this->angle_z += angle_z;
    this->scale *= (s+1);
}

double TriMesh::is_hit(Vector3d ray_origin, Vector3d ray_direction)
{
    vector<Vector3d> vertices = this->faces;
    Matrix4f trans = get_trans_mat();
    for (int i=0; i<vertices.size(); i=i+3){
        MatrixXf points(4,3);
        points.col(0) << vertices[i](0),vertices[i](1),vertices[i](2),1;
        points.col(1) << vertices[i+1](0),vertices[i+1](1),vertices[i+1](2),1;
        points.col(2) << vertices[i+2](0),vertices[i+2](1),vertices[i+2](2),1;
        MatrixXf res = trans*points;
        
        double a_x = res(0,0); double a_y = res(1,0); double a_z = res(2,0);
        double b_x = res(0,1); double b_y = res(1,1); double b_z = res(2,1);
        double c_x = res(0,2); double c_y = res(1,2); double c_z = res(2,2);
        double e_x = ray_origin[0];    double e_y = ray_origin[1];    double e_z = ray_origin[2];
        double d_x = ray_direction[0]; double d_y = ray_direction[1]; double d_z = ray_direction[2];
    
        double A11 = a_x-b_x; double A12 = a_x-c_x;
        double A21 = a_y-b_y; double A22 = a_y-c_y;
        double A31 = a_z-b_z; double A32 = a_z-c_z;
        double Xae = a_x-e_x; double Yae = a_y-e_y; double Zae = a_z-e_z;
        double Axd_sub = A21*A32-A22*A31;
        double Ayd_sub = A11*A32-A12*A31;
        double Azd_sub = A11*A22-A21*A12;
    
        double A = d_x*Axd_sub - d_y*Ayd_sub + d_z*Azd_sub;
        double t = (Xae*Axd_sub - Yae*Ayd_sub +  Zae*Azd_sub)/A;
        if (t<=0){
            continue;
        }
    
        double gamma = (d_x * (A21*Zae-Yae*A31) - d_y * (A11*Zae-Xae*A31) + d_z * (A11*Yae-Xae*A21))/A;
        if (gamma<0 || gamma>1){
            continue;
        }
    
        double beta = (d_x * (Yae*A32-Zae*A22)-d_y * (Xae*A32-Zae*A12) + d_z * (Xae*A22-Yae*A12))/A;
        if(beta<0 || beta>1-gamma){
            continue;
        }
        return t;
    }
    return -1;
}

Vector3d TriMesh::get_color(){
    return color;
}

void TriMesh::set_color(Vector3d c){
    color = c;
}

