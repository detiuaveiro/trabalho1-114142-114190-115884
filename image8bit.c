/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec:114190  Name:Rúben Costa
// NMec:115884  Name:Lázaro Sá
//
//
// Date: 14/11/2023
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
//
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image
{
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8 *pixel; // pixel data (a raster scan)
};

// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
//
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char *errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char *ImageErrMsg()
{ ///
  return errCause;
}

// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success =
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
//
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
//
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
//
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)

// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char *failmsg)
{
  errCause = (char *)(condition ? "" : failmsg);
  return condition;
}

/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void)
{ ///
  InstrCalibrate();
  InstrName[0] = "pixmem"; // InstrCount[0] will count pixel array acesses
  // Name other counters here...
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval)
{ ///
  assert(width >= 0);
  assert(height >= 0);
  assert(0 < maxval && maxval <= PixMax);

  // Allocate memory for the image structure
  Image img = (Image)malloc(sizeof(struct image));

  if (img == NULL)
  {
    errCause = "Memory allocation failed";
    return NULL;
  }

  // Set the image properties
  img->width = width;
  img->height = height;
  img->maxval = maxval;

  // Allocate memory for the pixel array
  img->pixel = (uint8 *)malloc(width * height * sizeof(uint8));

  if (img->pixel == NULL)
  {
    // Memory allocation failed, clean up and return NULL
    free(img);
    errCause = "Memory allocation for pixel array failed";
    return NULL;
  }

  return img;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image *imgp)
{ ///
  assert(imgp != NULL);

  if (*imgp != NULL)
  {
    // Free the pixel array
    free((*imgp)->pixel);
    // Free the image structure
    free(*imgp);

    *imgp = NULL;
  }
}

/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE *f)
{
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n')
  {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char *filename)
{ ///
  int w, h;
  int maxval;
  char c;
  FILE *f = NULL;
  Image img = NULL;

  int success =
      check((f = fopen(filename, "rb")) != NULL, "Open failed") &&
      // Parse PGM header
      check(fscanf(f, "P%c ", &c) == 1 && c == '5', "Invalid file format") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d ", &h) == 1 && h >= 0, "Invalid height") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax, "Invalid maxval") &&
      check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected") &&
      // Allocate image
      (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
      // Read pixels
      check(fread(img->pixel, sizeof(uint8), w * h, f) == w * h, "Reading pixels");
  PIXMEM += (unsigned long)(w * h); // count pixel memory accesses

  // Cleanup
  if (!success)
  {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL)
    fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char *filename)
{ ///
  assert(img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE *f = NULL;

  int success =
      check((f = fopen(filename, "wb")) != NULL, "Open failed") &&
      check(fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed") &&
      check(fwrite(img->pixel, sizeof(uint8), w * h, f) == w * h, "Writing pixels failed");
  PIXMEM += (unsigned long)(w * h); // count pixel memory accesses

  // Cleanup
  if (f != NULL)
    fclose(f);
  return success;
}

/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img)
{ ///
  assert(img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img)
{ ///
  assert(img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img)
{ ///
  assert(img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8 *min, uint8 *max)
{ ///
  assert(img != NULL);
  // minimum gray -> black (0)
  // maximum gray -> white (255)

  // atribuir o maior valor ao min para cada vez que quando encontrar um valor menor substituir
  *min = PixMax;
  // atribuir o menor valor ao max para cada vez que quando encontrar um valor maior substituir
  *max = 0;

  for (int i = 0; i < img->width * img->height; i++)
  {
    // buscar o valor de gray levels do array de pixeis da imagem
    uint8 pixelValue = img->pixel[i];

    //encontrar o pixel com menor e maior valor
    if (pixelValue < *min)
    {
      *min = pixelValue;
    }
    if (pixelValue > *max)
    {
      *max = pixelValue;
    }
  }
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y)
{ ///
  assert(img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h)
{ ///
  assert(img != NULL);
  // Check if the starting position (x, y) is valid
  if (!ImageValidPos(img, x, y))
  {
    return 0; // Return false if starting position is invalid
  }
  // see how far the rectangle stretches form it's starting point
  int maxW = x + w;
  int maxH = y + h;

  if (maxW <= img->width && maxH <= img->height)
  {
    return 1; // Return true if the entire area is inside the image
  }
  else
  {
    return 0; // otherwise return false
  }
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel.
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y)
{
  int index;
  // fórmula:
  //'y * img->width' calcula a posição vertical
  //'+ x' ajusta essa posição horizontalmente para a coluna
  index = y * img->width + x;
  assert(0 <= index && index < img->width * img->height);
  return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y)
{ ///
  assert(img != NULL);
  assert(ImageValidPos(img, x, y));
  PIXMEM += 1; // count one pixel access (read)
  return img->pixel[G(img, x, y)];
}

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level)
{ ///
  assert(img != NULL);
  assert(ImageValidPos(img, x, y));
  PIXMEM += 1; // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
}

/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.

/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img)
{ ///
  assert(img != NULL);

  // Obtém o valor máximo do pixel (branco)
  uint8 maxPixelValue = ImageMaxval(img);

  // Percorre todos os pixels da imagem
  for (int y = 0; y < ImageHeight(img); y++)
  {
    for (int x = 0; x < ImageWidth(img); x++)
    {
      // Obtém o valor do pixel atual
      uint8 pixelValue = ImageGetPixel(img, x, y);

      // Calcula o valor negativo (inverte o valor do pixel)
      uint8 negativeValue = maxPixelValue - pixelValue;

      // Define o novo valor do pixel
      ImageSetPixel(img, x, y, negativeValue);
    }
  }
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr)
{ ///
  assert(img != NULL);

  // Percorre todos os pixels da imagem
  for (int y = 0; y < ImageHeight(img); y++)
  {
    for (int x = 0; x < ImageWidth(img); x++)
    {
      // Obtém o valor do pixel atual
      uint8 pixelValue = ImageGetPixel(img, x, y);

      // verifica se o pixel é inferior a thr
      if (pixelValue < thr)
      {
        // transforma de branco para preto
        ImageSetPixel(img, x, y, 0);
      }
      else
      {
        // inverso (preto para branco)
        ImageSetPixel(img, x, y, 255);
      }
    }
  }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor)
{ ///
  assert(img != NULL);
  assert(factor >= 0.0);

  // Percorre todos os pixels da imagem
  for (int y = 0; y < ImageHeight(img); y++)
  {
    for (int x = 0; x < ImageWidth(img); x++)
    {
      // Obtém o valor do pixel atual
      uint8 pixelValue = ImageGetPixel(img, x, y);

      // Calcula o novo valor do pixel multiplicando pelo fator
      int NewpixelValue = (int)(pixelValue * factor + 0.5);
      // verifica se vai ultrpassar o maxVal (se for adquire o valor de maxVal)
      if (NewpixelValue > 255)
      {
        // satura para o maior valor possivel (maxVal = 255)
        NewpixelValue = 255;
      }
      if (NewpixelValue < 0)
      {
        // satura para o menor valor possivel (0)
        NewpixelValue = 0;
      }

      // define o novo valor do pixel
      ImageSetPixel(img, x, y, (uint8)NewpixelValue);
    }
  }
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
///
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint:
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees clockwise.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img)
{ /// rotação de 90º, a coordenada x mantem-se, o que muda é a y
  // 1 2 3           7 4 1
  // 4 5 6    =>     8 5 2
  // 7 8 9           9 6 3

  assert(img != NULL);

  // Cria uma nova imagem
  Image rotatedImg = ImageCreate(img->height, img->width, img->maxval);

  // Verifica se a criação da nova imagem foi bem-sucedida
  if (rotatedImg == NULL)
  {
    errCause = "Memory allocation failed";
    return NULL;
  }

  for (int y = 0; y < ImageHeight(img); y++)
  {
    for (int x = 0; x < ImageWidth(img); x++)
    {
      // calcular as coordenadas após a rotação
      // examples: (2,1)->(3,2) ; (1,2)->(2,1) nota que o x é igual ao rotatedY

      // Get the pixel value from the original image
      uint8 pixelValue = ImageGetPixel(img, x, y);

      // Calculate the new coordinates after rotation
      int newX = y;
      int newY = img->width - 1 - x;

      // Set the pixel value in the rotated image
      ImageSetPixel(rotatedImg, newX, newY, pixelValue);
    }
  }

  return rotatedImg;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img)
{ /// no flip left-right, o y mantém-se
  // 1 2 3           3 2 1
  // 4 5 6    =>     6 5 4
  // 7 8 9           9 8 7
  assert(img != NULL);

  // Cria uma nova imagem
  Image mirrorImg = ImageCreate(img->height, img->width, img->maxval);

  // Verifica se a criação da nova imagem foi bem-sucedida
  if (mirrorImg == NULL)
  {
    errCause = "Memory allocation failed";
    return NULL;
  }

  for (int y = 0; y < ImageHeight(img); y++)
  {
    for (int x = 0; x < ImageWidth(img); x++)
    {
      // calcular as coordenadas após o flip left-right
      // examples: (1,1)->(3,1) ; (1,2)->(3,2) nota que o y mantém-se
      // Get the pixel value from the original image
      uint8 pixelValue = ImageGetPixel(img, x, y);

      // Calculate the new x-coordinate after mirroring
      int newX = img->width - 1 - x;

      // Set the pixel value in the mirrored image
      ImageSetPixel(mirrorImg, newX, y, pixelValue);
    }
  }

  return mirrorImg;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

Image ImageCrop(Image img, int x, int y, int w, int h)
{ ///
  assert(img != NULL);
  // verifica se a imagem vai ter um formato de um retângulo (validação)
  if (!ImageValidRect(img, x, y, w, h))
  {
    errCause = "Invalid img";
    return NULL;
  }

  // Create a new image for the cropped area
  Image croppedImg = ImageCreate(w, h, img->maxval);
  if (croppedImg == NULL)
  {
    errCause = "Memory allocation failed";
    return NULL;
  }

  // Copy the pixels from the original image to the cropped image
  for (int i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++)
    {
      // buscar o pixel e substituir na posição certa
      ImageSetPixel(croppedImg, j, i, ImageGetPixel(img, x + j, y + i));
    }
  }

  return croppedImg;
}

/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).

void ImagePaste(Image img1, int x, int y, Image img2)
{ ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  // make sure img 2 fits in img 1
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));

  //percorre a imagem toda e encontra a posição onde quer colar a imagem
  //quando encontrada começa a substituir os pixeis da img1 pelos da img2
  for (int i = 0; i < img2->height; i++)
  {
    for (int j = 0; j < img2->width; j++)
    {
      ImageSetPixel(img1, x + j, y + i, ImageGetPixel(img2, j, i));
    }
  }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha)
{ ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));

  for (int i = 0; i < img2->height; i++)
  {
    for (int j = 0; j < img2->width; j++)
    {
      // Obtenha os valores dos pixels das duas imagens
      // encontrar as posições dos pixeis da img 1 onde vai ficar os pixeis da img 2 e obte-los
      uint8 pixelImg1 = ImageGetPixel(img1, x + j, y + i);
      uint8 pixelImg2 = ImageGetPixel(img2, j, i);

      // defenir o valor do novo pixel (using a formula)
      // formula: blendedValue = α    * pixelImg2 + (1.0 − α    ) * pixelImg1
      int blendedValue = (int)(alpha * pixelImg2 + (1.0 - alpha) * pixelImg1 + 0.5);

      // caso o blendedValue não esteja nos limites dos valores posiveis de um pixel (0 a 255)
      // ajustar a saturação
      if (blendedValue > 255)
      { // ou blendedValue = (blendedValue > 255) ? 255 : blendedValue;
        blendedValue = 255;
      }

      if (blendedValue < 0)
      { // ou blendedValue = (blendedValue < 0) ? 0 : blendedValue;
        blendedValue = 0;
      }

      // defenir o pixel
      ImageSetPixel(img1, x + j, y + i, (uint8)blendedValue);
    }
  }
}

/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.

int count = 0;

int ImageMatchSubImage(Image img1, int x, int y, Image img2)
{ ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidPos(img1, x, y));

  for (int i = 0; i < img2->height; i++)
  {
    for (int j = 0; j < img2->width; j++)
    {
      count += 1;
      if (ImageGetPixel(img1, x + j, y + i) != ImageGetPixel(img2, j, i))
      {
        // Os pixels não são idênticos => não há correspondência
        return 0;
      }
    }
  }
  return 1;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int *px, int *py, Image img2)
{ ///
  assert(img1 != NULL);
  assert(img2 != NULL);

  for (int i = 0; i <= img1->height - img2->height + 1; i++)
  {
    for (int j = 0; j <= img1->width - img2->width + 1; j++)
    {
      if (ImageMatchSubImage(img1, j, i, img2))
      {
        // Correspondência encontrada, atualiza as posições e retorna 1
        *px = j;
        *py = i;
        printf("Total de comparações feitas: %d\n", count);
        return 1;
      }
    }
  }
  printf("Total de comparações feitas: %d\n", count);
  return 0;
}

/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.
void ImageBlur(Image img, int dx, int dy)
{ ///
  assert(img != NULL);
  assert(dx >= 0 && dy >= 0);

  Image tempImg = ImageCreate(img->width, img->height, img->maxval);

  // Verificar se a criação da imagem temporária foi bem-sucedida
  if (tempImg == NULL)
  {
    errCause = "Memory allocation failed";
    return;
  }

  for (int y = 0; y < ImageHeight(img); y++)
  {
    for (int x = 0; x < ImageWidth(img); x++)
    {
      int sum = 0;
      int count = 0;

      // Percorre a vizinhança definida por dx e dy
      //começa em negativo para que percorra para cima e para a esquerda
      for (int newY = y - dy; newY <= y + dy; newY++)
      {
        for (int newX = x - dx; newX <= x + dx; newX++)
        {
          // Verifica se as coordenadas estão dentro dos limites da imagem
          if (ImageValidPos(img, newX, newY))
          {
            //acumular o valor dos pixeis da vizinhança
            sum += ImageGetPixel(img, newX, newY);
            //numero de pixeis contados
            count++;
          }
        }
      }
      
      // Calcula a média e define o novo valor do pixel na imagem original
      int avgValue = count > 0 ? (sum + count/2)/ count : 0;
      ImageSetPixel(tempImg, x, y, avgValue);
    }
  }
  ImagePaste(img, 0, 0, tempImg);
  // Libera a imagem temporária
  ImageDestroy(&tempImg);
  InstrPrint();
}