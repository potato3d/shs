#ifndef __IMAGE_DILATION__
#define __IMAGE_DILATION__

//#include <OGF/image/types/image_library.h>
//#include <OGF/image/types/image.h>
#include <iostream>

class OGFImage
{
public:
	OGFImage( int width, int height );
	OGFImage( const OGFImage* other ); // deep copy
	~OGFImage();

	int width() const;
	int height() const;
	float* pixel_base( int col, int row );
	float* base_mem();
	int num_channels();
	int bytes_per_pixel();

private:
	int _width;
	int _height;
	float* _pixels;
};

OGFImage::OGFImage( int width, int height )
: _width( width ), _height( height )
{
	_pixels = new float[_width*_height*4];
}

OGFImage::OGFImage( const OGFImage* other )
: _width( other->_width ), _height( other->_height )
{
	_pixels = new float[_width*_height*4];
	std::copy( other->_pixels, other->_pixels + _width*_height*num_channels(), _pixels );
}

OGFImage::~OGFImage()
{
	delete[] _pixels;
}

int OGFImage::width() const
{
	return _width;
}

int OGFImage::height() const
{
	return _height;
}

float* OGFImage::pixel_base( int col, int row )
{
	return &_pixels[num_channels()*(row*_width+col)];
}

float* OGFImage::base_mem()
{
	return _pixels;
}

int OGFImage::num_channels()
{
	return 4;
}

int OGFImage::bytes_per_pixel()
{
	return sizeof(float)*num_channels();
}


//alpha
//has_neighbor
//DDx //DD_x //DDy //DD_y
//DDxy //DDx_y //DD_xy //DD_x_y 

OGFImage* global_RGBAimg;

inline float alpha(int x, int y)
{
  return global_RGBAimg->pixel_base(x, y)[3];
}

bool has_neighbor(int x, int y)
{
  int x_begin = (x > 0) ? x-1 : 0;
  int y_begin = (y > 0) ? y-1 : 0;
  int x_end = (x < global_RGBAimg->width()-1) ? x+1 : global_RGBAimg->width()-1 ;
  int y_end = (y < global_RGBAimg->height()-1) ? y+1 : global_RGBAimg->height()-1 ;
  for (int i=x_begin; i<=x_end; i++)
  {
    for (int j=y_begin; j<=y_end; j++)
    {
      if (i==x && j==y)
        continue; //to not consider itself 
      if (alpha(i,j)>0.0)
        return true;
    }
  }
  return false;
}

float diff(int base_x, int base_y, int other_x, int other_y, int channel = 0)  // channel = [0,3]
{
  //assert x y inside [0,width/height]
  if (other_x >= global_RGBAimg->width()  || 
      other_y >= global_RGBAimg->height() || 
      other_x < 0 || other_y < 0)
    return 0;
  if( alpha(base_x,base_y) < 1.0f || alpha(other_x,other_y) < 1.0f )
    return 0;
  return global_RGBAimg->pixel_base( base_x, base_y )[channel] - 
         global_RGBAimg->pixel_base( other_x, other_y )[channel];
}

float diff_sum(int base_x, int base_y, int other_x, int other_y, int channel = 0)  // channel = [0,3]
{
  return diff(base_x, base_y, other_x, other_y, channel) +
         global_RGBAimg->pixel_base(base_x, base_y)[channel];
}

void image_dilation_one_ring()
{
  //global_RGBAimg  >> tmp
  OGFImage* tmp = new OGFImage(global_RGBAimg) ;
  int count_changed_pixels = 0;

  for (int line = 0; line < global_RGBAimg->height(); line++)
  {
    for (int col = 0; col < global_RGBAimg->width(); col++)
	{
      if (alpha(col, line) == 0 && has_neighbor(col,line))   //a to-be-dilated pixel
        {
          float v[3], w;
          v[0]=v[1]=v[2]=w=0.0f;
          int shift_x_begin = (col > 0) ? -1 : 0;
          int shift_y_begin = (line > 0) ? -1 : 0;
          int shift_x_end = (col < global_RGBAimg->width()-1) ? 1 : 0 ;
          int shift_y_end = (line < global_RGBAimg->height()-1) ? 1 : 0 ;
          //for all neighbors (excluding those that doesn't exist because of borders)
          for (int shift_x=shift_x_begin; shift_x<=shift_x_end; shift_x++)
		  {
            for (int shift_y=shift_y_begin; shift_y<=shift_y_end; shift_y++)
            {
              if (shift_x==0 && shift_y==0)
                continue; //to not consider itself 
              int nx = col+shift_x;
              int ny = line+shift_y;
              if ( alpha(nx,ny) > 0.0f )  //valid neighbor
              {
                double weight = 1.0/sqrt((double)shift_x*shift_x+shift_y*shift_y);
                for (int ch=0; ch<3; ch++) //r,g,b
                  v[ch] += weight*diff_sum(col+shift_x, line+shift_y, col+2*shift_x, line+2*shift_y, ch);
                w += weight;
              }
            }
		  }
            for (int ch=0; ch<3; ch++) //r,g,b
            {
              float final = (v[ch]/w);
              if (final > 1.0f)
				  final = 1.0f;
              if (final < -1.0f)
				  final = -1.0f;
              tmp->pixel_base(col,line)[ch] = final;
            }
          tmp->pixel_base(col,line)[3] = 1.0f;
          count_changed_pixels++;
        }
	}
  }
  //tmp >> global_RGBAimg
  //OGF::Memory::copy(   //(to, from, size)
  //  global_RGBAimg->base_mem(), tmp->base_mem(), 
  //  global_RGBAimg->width() * global_RGBAimg->height() * 
  //  global_RGBAimg->bytes_per_pixel()
  //  ) ;

  //memcpy( global_RGBAimg->base_mem(), tmp->base_mem(), global_RGBAimg->width() * global_RGBAimg->height() * global_RGBAimg->bytes_per_pixel() );
  std::copy( tmp->base_mem(), tmp->base_mem() + tmp->width()*tmp->height()*tmp->num_channels(), global_RGBAimg->base_mem() );

  std::cout << "IMAGE TO DILATE" <<  " Pixels changed "  << count_changed_pixels << std::endl ;

  delete tmp;
}

void image_dilation(OGFImage* RGBAimg,int dilate_width_)
{
  //if (RGBAimg->color_encoding() != OGF::Image::RGBA || RGBAimg->bytes_per_pixel() != 4)
  //  return;
  global_RGBAimg = RGBAimg;
  for (int i = 0; i< dilate_width_; i++)
    image_dilation_one_ring();
  return;
}


#endif

//BEGIN DERIVATIVES
//
//
//      x          DDx_y     DD_y    DD_x_y
//  o---->              \     |     /
//  |                    \    |    /
//  | y                   \   |   /
//  V              DDx  ---((x,y))---  DD_x
//                        /   |   \
//                       /    |    \
//                      /     |     \
//                  DDxy     DDy     DD_xy
//int DDx(int x, int y, int channel = 0)  // channel = [0,3]
//{
//  if (x == 0 || alpha(x,y)<1 || alpha(x-1,y)<1)
//    return 0;
//  return global_RGBAimg->pixel_base(x, y)[channel] - 
//    global_RGBAimg->pixel_base(x-1, y)[channel];
//}
//
//int DDy(int x, int y, int channel = 0)  // channel = [0,3]
//{
//  if (y == 0 || alpha(x,y)<1 || alpha(x,y-1)<1)
//    return 0;
//  return global_RGBAimg->pixel_base(x, y)[channel] - 
//    global_RGBAimg->pixel_base(x, y-1)[channel];
//}
//
//int DD_x(int x, int y, int channel = 0)  // channel = [0,3]
//{
//  if (x == global_RGBAimg->width()-1 || alpha(x,y)<1 || alpha(x+1,y)<1)
//    return 0;
//  return global_RGBAimg->pixel_base(x, y)[channel] - 
//    global_RGBAimg->pixel_base(x+1, y)[channel];
//}
//
//int DD_y(int x, int y, int channel = 0)  // channel = [0,3]
//{
//  if (y == global_RGBAimg->height()-1 || alpha(x,y)<1 || alpha(x,y+1)<1)
//    return 0;
//  return global_RGBAimg->pixel_base(x, y)[channel] - 
//    global_RGBAimg->pixel_base(x, y+1)[channel];
//}
//
//int DDxy(int x, int y, int channel = 0)  // channel = [0,3]
//{
//  if (x == 0 || y == 0 || alpha(x,y)<1 || alpha(x-1,y-1)<1)
//    return 0;
//  return global_RGBAimg->pixel_base(x, y)[channel] - 
//    global_RGBAimg->pixel_base(x-1, y-1)[channel];
//}
//
//int DD_xy(int x, int y, int channel = 0)  // channel = [0,3]
//{
//  if (x == global_RGBAimg->width()-1 || y == 0 || alpha(x,y)<1 || alpha(x+1,y-1)<1)
//    return 0;
//  return global_RGBAimg->pixel_base(x, y)[channel] - 
//    global_RGBAimg->pixel_base(x+1, y-1)[channel];
//}
//
//int DDx_y(int x, int y, int channel = 0)  // channel = [0,3]
//{
//  if (x == 0 || y == global_RGBAimg->height()-1  || alpha(x,y)<1 || alpha(x-1,y+1)<1)
//    return 0;
//  return global_RGBAimg->pixel_base(x, y)[channel] - 
//    global_RGBAimg->pixel_base(x-1, y+1)[channel];
//}
//
//int DD_x_y(int x, int y, int channel = 0)  // channel = [0,3]
//{
//  if (x == global_RGBAimg->width()-1 || y == global_RGBAimg->height()-1  || alpha(x,y)<1 || alpha(x+1,y+1)<1)
//    return 0;
//  return global_RGBAimg->pixel_base(x, y)[channel] - 
//    global_RGBAimg->pixel_base(x+1, y+1)[channel];
//}
//END DERIVATIVES
