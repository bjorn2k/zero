#include "glwidget.h"
#include <cstdlib>
#include <QMouseEvent>
#include <QDebug>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

GLWidget::GLWidget(QWidget *parent)
: add(false)
, del(false)
, color(false)
, xrot(0.0)
, QGLWidget(parent)
, zoom(0.0f)
, selectedcolor(glm::vec3(0.0f, 0.0f, 1.0f))
, extrudedirection(0)
, extrudesize(0)
{
        setMouseTracking(true);
        // Next line is used to get keyboard events to this Widget.
        setFocusPolicy(Qt::StrongFocus);
        std::srand(time(0));
        for(size_t z = 0; z < size; z++)
        {
                for(size_t y = 0; y < size; y++)
                {
                        for(size_t x = 0; x < size; x++)
                        {
                                cellenabled[x][y][z] = false;
                                colors[x][y][z].x = (float)std::rand() / RAND_MAX;
                                colors[x][y][z].y = (float)std::rand() / RAND_MAX;
                                colors[x][y][z].z = (float)std::rand() / RAND_MAX;
                                selected[x][y][z] = false;
                        }
                }
        }
        cellenabled[size/2][size/2][size/2] = true;
}

void GLWidget::initializeGL()
{
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glClearDepth(1.0f);
        glEnable(GL_POLYGON_SMOOTH);

        glShadeModel(GL_SMOOTH);

        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        // Create light components
        float ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        float diffuseLight[] = { 0.8f, 0.8f, 0.8, 1.0f };
        float specularLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        float position[] = { 0.0f, 0.0f, 0.0f, 1.0f };

        // Assign created components to GL_LIGHT0
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
        glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
        glLightfv(GL_LIGHT0, GL_POSITION, position);
        glEnable(GL_LIGHT0);
        glClearColor(0, 0, 0, 0);
}

void GLWidget::resizeGL(int w, int h)
{
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glm::mat4 projection = glm::perspective(60.0f, w/(float)h, 1.0f, 1000.0f);
        glLoadMatrixf(&projection[0][0]);
}

void GLWidget::paintGL() {
        glMatrixMode(GL_MODELVIEW);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 view  = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -((1.6 + zoom) * size)));
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), xrot, glm::vec3(0.0f, -1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), yrot, glm::vec3(-1.0f, 0.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-size/2.0f, -size/2.0f, -size/2.0f));
        glLoadMatrixf(&(view * model)[0][0]);
        glEnable(GL_LIGHTING);

        glBegin(GL_TRIANGLES);
        for(size_t z = 0; z < size; z++)
        {
                for(size_t y = 0; y < size; y++)
                {
                        for(size_t x = 0; x < size; x++)
                        {
                                if(cellenabled[x][y][z])
                                {
                                        if(selected[x][y][z])
                                        {
                                                drawCube(x, y, z, glm::vec3(1.0f, 0.0f, 0.0f));
                                        }
                                        else
                                        {
                                                drawCube(x, y, z, colors[x][y][z]);
                                        }
                                }
                        }
                }
        }
        for(size_t z = 0; z < size; z++)
        {
                for(size_t y = 0; y < size; y++)
                {
                        for(size_t x = 0; x < size; x++)
                        {
                                if(selected[x][y][z])
                                {
                                        int begin = 0;
                                        int end = extrudesize;
                                        if(begin > end)
                                        {
                                                begin = extrudesize;
                                                end = 0;
                                        }
                                        for(int offset = begin; offset <= end; offset++)
                                        {
                                                switch (extrudedirection)
                                                {
                                                        //x-1 case;
                                                        case 0:
                                                                drawCube(x - offset, y, z, glm::vec3(1.0f, 0.0f, 0.0f));
                                                        break;
                                                        //z+1 case;
                                                        case 1:
                                                                drawCube(x, y, z + offset, glm::vec3(1.0f, 0.0f, 0.0f));
                                                        break;
                                                        //x+1 case;
                                                        case 2:
                                                                drawCube(x + offset, y, z, glm::vec3(1.0f, 0.0f, 0.0f));
                                                        break;
                                                        //z-1 case;
                                                        case 3:
                                                                drawCube(x, y, z - offset, glm::vec3(1.0f, 0.0f, 0.0f));
                                                        break;
                                                        //y+1 case;
                                                        case 4:
                                                                drawCube(x, y + offset, z, glm::vec3(1.0f, 0.0f, 0.0f));
                                                        break;
                                                        //y-1 case;
                                                        case 5:
                                                                drawCube(x, y - offset, z, glm::vec3(1.0f, 0.0f, 0.0f));
                                                        break;
                                                };
                                        }
                                }
                        }
                }
        }
        glEnd();
}

void GLWidget::selectionRender(bool sides)
{
        glMatrixMode(GL_MODELVIEW);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 view  = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -((1.6f + zoom) * size)));
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), xrot, glm::vec3(0.0f, -1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), yrot, glm::vec3(-1.0f, 0.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-size/2.0f, -size/2.0f, -size/2.0f));
        glLoadMatrixf(&(view * model)[0][0]);

        glDisable(GL_LIGHTING);
        glBegin(GL_TRIANGLES);
        for(size_t z = 0; z < size; z++)
        {
                for(size_t y = 0; y < size; y++)
                {
                        for(size_t x = 0; x < size; x++)
                        {
                                if(cellenabled[x][y][z])
                                {
                                        glColor3f((x + 1) / (float)255, (y + 1) / (float)255, (z + 1) / (float)255);
                                        if(sides) glColor3f(1 / (float)255, 0, 0);
                                        glVertex3f( 0 + x,  1 + y,  0 + z);
                                        glVertex3f( 0 + x,  0 + y,  1 + z);
                                        glVertex3f( 0 + x,  0 + y,  0 + z);

                                        glVertex3f( 0 + x,  0 + y,  1 + z);
                                        glVertex3f( 0 + x,  1 + y,  0 + z);
                                        glVertex3f( 0 + x,  1 + y,  1 + z);

                                        if(sides) glColor3f(2 / (float)255, 0, 0);
                                        glVertex3f( 0 + x,  1 + y,  1 + z);
                                        glVertex3f( 1 + x,  0 + y,  1 + z);
                                        glVertex3f( 0 + x,  0 + y,  1 + z);

                                        glVertex3f( 1 + x,  0 + y,  1 + z);
                                        glVertex3f( 0 + x,  1 + y,  1 + z);
                                        glVertex3f( 1 + x,  1 + y,  1 + z);

                                        if(sides) glColor3f(3 / (float)255, 0, 0);
                                        glVertex3f( 1 + x,  0 + y,  1 + z);
                                        glVertex3f( 1 + x,  1 + y,  0 + z);
                                        glVertex3f( 1 + x,  0 + y,  0 + z);

                                        glVertex3f( 1 + x,  1 + y,  0 + z);
                                        glVertex3f( 1 + x,  0 + y,  1 + z);
                                        glVertex3f( 1 + x,  1 + y,  1 + z);

                                        if(sides) glColor3f(4 / (float)255, 0, 0);
                                        glVertex3f( 1 + x,  1 + y,  0 + z);
                                        glVertex3f( 0 + x,  0 + y,  0 + z);
                                        glVertex3f( 1 + x,  0 + y,  0 + z);

                                        glVertex3f( 0 + x,  0 + y,  0 + z);
                                        glVertex3f( 1 + x,  1 + y,  0 + z);
                                        glVertex3f( 0 + x,  1 + y,  0 + z);

                                        if(sides) glColor3f(5 / (float)255, 0, 0);
                                        glVertex3f( 0 + x,  1 + y,  0 + z);
                                        glVertex3f( 1 + x,  1 + y,  1 + z);
                                        glVertex3f( 0 + x,  1 + y,  1 + z);

                                        glVertex3f( 1 + x,  1 + y,  1 + z);
                                        glVertex3f( 0 + x,  1 + y,  0 + z);
                                        glVertex3f( 1 + x,  1 + y,  0 + z);

                                        if(sides) glColor3f(6 / (float)255, 0, 0);
                                        glVertex3f( 1 + x,  0 + y,  0 + z);
                                        glVertex3f( 0 + x,  0 + y,  1 + z);
                                        glVertex3f( 1 + x,  0 + y,  1 + z);

                                        glVertex3f( 0 + x,  0 + y,  1 + z);
                                        glVertex3f( 1 + x,  0 + y,  0 + z);
                                        glVertex3f( 0 + x,  0 + y,  0 + z);
                                }
                        }
                }
        }
        glEnd();
}

void GLWidget::drawCube(size_t x, size_t y, size_t z, glm::vec3 color)
{
        if(x < 0 || x >= size || y < 0 || y >= size || z < 0 || z >= size) return;
        glColor3f(color.x, color.y, color.z);
        glNormal3f(-1, 0, 0);
        glVertex3f( 0 + x,  1 + y,  0 + z);
        glVertex3f( 0 + x,  0 + y,  1 + z);
        glVertex3f( 0 + x,  0 + y,  0 + z);

        glVertex3f( 0 + x,  0 + y,  1 + z);
        glVertex3f( 0 + x,  1 + y,  0 + z);
        glVertex3f( 0 + x,  1 + y,  1 + z);

        glNormal3f(0, 0, 1);
        glVertex3f( 0 + x,  1 + y,  1 + z);
        glVertex3f( 1 + x,  0 + y,  1 + z);
        glVertex3f( 0 + x,  0 + y,  1 + z);

        glVertex3f( 1 + x,  0 + y,  1 + z);
        glVertex3f( 0 + x,  1 + y,  1 + z);
        glVertex3f( 1 + x,  1 + y,  1 + z);

        glNormal3f(1, 0, 0);
        glVertex3f( 1 + x,  0 + y,  1 + z);
        glVertex3f( 1 + x,  1 + y,  0 + z);
        glVertex3f( 1 + x,  0 + y,  0 + z);

        glVertex3f( 1 + x,  1 + y,  0 + z);
        glVertex3f( 1 + x,  0 + y,  1 + z);
        glVertex3f( 1 + x,  1 + y,  1 + z);

        glNormal3f(0, 0, -1);
        glVertex3f( 1 + x,  1 + y,  0 + z);
        glVertex3f( 0 + x,  0 + y,  0 + z);
        glVertex3f( 1 + x,  0 + y,  0 + z);

        glVertex3f( 0 + x,  0 + y,  0 + z);
        glVertex3f( 1 + x,  1 + y,  0 + z);
        glVertex3f( 0 + x,  1 + y,  0 + z);

        glNormal3f(0, 1, 0);
        glVertex3f( 0 + x,  1 + y,  0 + z);
        glVertex3f( 1 + x,  1 + y,  1 + z);
        glVertex3f( 0 + x,  1 + y,  1 + z);

        glVertex3f( 1 + x,  1 + y,  1 + z);
        glVertex3f( 0 + x,  1 + y,  0 + z);
        glVertex3f( 1 + x,  1 + y,  0 + z);

        glNormal3f(0, -1, 0);
        glVertex3f( 1 + x,  0 + y,  0 + z);
        glVertex3f( 0 + x,  0 + y,  1 + z);
        glVertex3f( 1 + x,  0 + y,  1 + z);

        glVertex3f( 0 + x,  0 + y,  1 + z);
        glVertex3f( 1 + x,  0 + y,  0 + z);
        glVertex3f( 0 + x,  0 + y,  0 + z);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
        if(event->buttons() & Qt::RightButton)
        {
                xWhenPressed = event->x();
                yWhenPressed = event->y();
        }
        if(event->buttons() & Qt::LeftButton)
        {
                selectionRender(false);
                QRgb point = grabFrameBuffer().pixel(QPoint(event->x(), event->y()));
                size_t x = qRed(point) - 1;
                size_t y = qGreen(point) - 1;
                size_t z = qBlue(point) - 1;
                if( x < size && y < size && z << size)
                {
                        selectionRender(true);
                        QRgb sideinfo = grabFrameBuffer().pixel(QPoint(event->x(), event->y()));
                        size_t side = qRed(sideinfo) - 1;
                        if(add)
                        {
                                switch (side)
                                {
                                        //x-1 case;
                                        case 0:
                                                if(x-1 >= 0) cellenabled[x-1][y][z] = true;
                                        break;
                                        //z+1 case;
                                        case 1:
                                                if(z < size) cellenabled[x][y][z+1] = true;
                                        break;
                                        //x+1 case;
                                        case 2:
                                                if(x < size) cellenabled[x+1][y][z] = true;
                                        break;
                                        //z-1 case;
                                        case 3:
                                                if(z-1 >= 0) cellenabled[x][y][z-1] = true;
                                        break;
                                        //y+1 case;
                                        case 4:
                                                if(y < size) cellenabled[x][y+1][z] = true;
                                        break;
                                        //y-1 case;
                                        case 5:
                                                if(y-1 >= 0) cellenabled[x][y-1][z] = true;
                                        break;
                                };
                        }
                        else if(del)
                        {
                                if( x < size && y < size && z << size) cellenabled[x][y][z] = false;
                        }
                        else if(color)
                        {
                                if( x < size && y < size && z << size) colors[x][y][z] = selectedcolor;
                        }
                        else if(selecting)
                        {
                                if( x < size && y < size && z << size && extrudedirection == 0)
                                {
                                        extrudedirection = side;
                                }
                                selected[x][y][z] = !selected[x][y][z];
                        }
                        qDebug() << x << ", " << y << ", " << z << ", " << side;
                }
        }
        updateGL();
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
        accumulatedRotx = xrot;
        accumulatedRoty = yrot;
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
        if(event->buttons() & Qt::RightButton)
        {
                xrot = ((xWhenPressed - event->x()) / 4.0f) + accumulatedRotx;
                yrot = ((yWhenPressed - event->y()) / 4.0f) + accumulatedRoty;
        }

        updateGL();
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
        if(!selecting)
        {
                zoom -= event->delta() / (float)512;
        }
        if(selecting)
        {
                extrudesize -= (int)(event->delta() / 120);
        }
        updateGL();
}

void GLWidget::keyPressEvent (QKeyEvent *event)
{
        if(event->key() == Qt::Key_A) add = true;
        if(event->key() == Qt::Key_S) color = true;
        if(event->key() == Qt::Key_D) del = true;
        if(event->key() == Qt::Key_Control)
        {
                selecting = true;
        }
}

void GLWidget::keyReleaseEvent(QKeyEvent *event)
{
        if(event->key() == Qt::Key_A) add = false;
        if(event->key() == Qt::Key_S) color = false;
        if(event->key() == Qt::Key_D) del = false;
        if(event->key() == Qt::Key_Control)
        {
                for(size_t z = 0; z < size; z++)
                {
                        for(size_t y = 0; y < size; y++)
                        {
                                for(size_t x = 0; x < size; x++)
                                {
                                        if(selected[x][y][z])
                                        {
                                                int begin = 0;
                                                int end = extrudesize;
                                                if(begin > end)
                                                {
                                                        begin = extrudesize;
                                                        end = 0;
                                                }
                                                for(int offset = begin; offset <= end; offset++)
                                                {
                                                        switch (extrudedirection)
                                                        {
                                                                //x-1 case;
                                                                case 0:
                                                                        cellenabled[x - offset][y][z] = true;
                                                                        colors[x - offset][y][z] = colors[x][y][z];
                                                                break;
                                                                //z+1 case;
                                                                case 1:
                                                                        cellenabled[x][y][z + offset] = true;
                                                                        colors[x][y][z + offset] = colors[x][y][z];
                                                                break;
                                                                //x+1 case;
                                                                case 2:
                                                                        cellenabled[x + offset][y][z] = true;
                                                                        colors[x + offset][y][z] = colors[x][y][z];
                                                                break;
                                                                //z-1 case;
                                                                case 3:
                                                                        cellenabled[x][y][z - offset] = true;
                                                                        colors[x][y][z - offset] = colors[x][y][z];
                                                                break;
                                                                //y+1 case;
                                                                case 4:
                                                                        cellenabled[x][y + offset][z] = true;
                                                                        colors[x][y + offset][z] = colors[x][y][z];
                                                                break;
                                                                //y-1 case;
                                                                case 5:
                                                                        cellenabled[x][y - offset][z] = true;
                                                                        colors[x][y - offset][z] = colors[x][y][z];
                                                                break;
                                                        };
                                                }
                                        }
                                        selected[x][y][z] = false;
                                }
                        }
                }
                extrudedirection = 0;
                extrudesize = 0;
                selecting = false;
        }
        updateGL();
}

