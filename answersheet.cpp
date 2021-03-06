#include "answersheet.h"
#include "ui_mainwindow.h"
#include <QZXing.h>
#include <asmOpenCV.h>


cv::Mat image,image_resized,orginal_img ,removed_color_img,empty;
const char* wndname = "Remove Colors";
std::vector<std::vector<cv::Vec3b>> omit_colors ;
int image_width,image_height;
void mouseHandler (int event, int x, int y, int flags, void *userdata);
int color_omit_rang = 25 ;
int thresh_choice_value = 40;
int max_cell = 10 ;
int eye_number ;
int pad_rect , referenceEye, distanceWidth, distanceHeight, choiceWidth, choiceHeight,
choiceNumber,  distanceChoiceChoice, numberOfQuestions,  columnDistance ,
barcodeX,barcodeY,barcodeWidth,barcodeHeight, eyes_darkness_threshold ,rowDistance,
code_refrenceEye ,code_numCode, code_distanceWidth, code_distanceHeight, code_distanceChoice, min_squre_size, max_squre_size;

bool rowQuestionOrder, has_code ;
cv::Size resize_size = cv::Size(1248,1755) ;
cv::Size orginal_size ;
QSqlDatabase db;

cv::Mat elementErode  = getStructuringElement( cv::MORPH_RECT,cv::Size(3,3), cv::Point(1,1));
cv::Mat elementDilate = getStructuringElement( cv::MORPH_RECT,cv::Size(5,5), cv::Point(2,2));


int temp_counter = 0 ;
int error_counter = 0 ;


std::vector<cv::Rect> left_eye,right_eye;
#define PI 3.14159265

bool AnswerSheet::createTable(QString tableName) {
    QSqlQuery query;
    return query.exec("create table " + tableName +
                      " (id integer PRIMARY KEY AUTOINCREMENT, "
                      "orginalFilePath varchar(300), "
                      "processedFilePath varchar(300), "
                      "code varchar(30) ,"
                      "answers varchar(300),date varchar(50),averageBlackness integer,choicesArea TEXT) "
                      );
}


bool AnswerSheet::deleteTable(QString tableName) {
    db.open() ;
    QSqlQuery query;
    return query.exec("DROP TABLE " +tableName+";") ;
}

bool AnswerSheet::clearTable(QString tableName) {
    db.open() ;
    QSqlQuery query;
    return query.exec("DELETE * FROM " +tableName) ;
}



const std::string currentDateTime() {
    QDateTime a = QDateTime::currentDateTime() ;
    return a.toString("yyyy-MM-dd hh,mm,ss,zzz").toStdString();
}

void AnswerSheet::openDB(QString dbName) {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbName);
    bool ok = db.open();
    if(ok)
        std::cout <<"Database Opend" << std::endl ;

}

AnswerSheet::AnswerSheet(cv::Mat img)
{
    orginal_size.width = img.cols ;
    orginal_size.height = img.rows ;
    img.copyTo(orginal_img);
    cv::resize(img,image,resize_size) ;
    image.copyTo(image_resized);
    image_width =  image.cols;
    image_height = image.rows;
    //    reader.reset(new MultiFormatReader);
}

cv::Mat AnswerSheet::RemoveColors() {
    RemoveColorsDialog();
    return image;

}
bool AnswerSheet::clearOmitedColors() {
    omit_colors.clear();
    return true ;
}

cv::Mat AnswerSheet::getImage() {
    return image ;
}

void AnswerSheet::DetectEyes (int pad_rectangle, int darkness_threshold,int min_squre, int max_squre) {
    image_resized.copyTo(image);
    omitColors(image);
    pad_rect =pad_rectangle ;
    eyes_darkness_threshold = darkness_threshold ;
    min_squre_size = min_squre ;
    max_squre_size = max_squre ;
    cv::Rect left_rect = cv::Rect(0,0,pad_rect,image.rows);
    cv::Rect right_rect = cv::Rect(image.cols-pad_rect,0,pad_rect,image.rows);
    cv::vector<cv::vector<cv::Point>> squares_left,squares_right;
    int a = findSquares(image, squares_left,left_rect);
    int b = findSquares(image, squares_right,right_rect);

    sortSquares(squares_left,left_eye,LEFT);
    sortSquares(squares_right,right_eye,RIGHT);

    if(a == b ) {
        eye_number =a ;

        double angle = findAngle(squares_left,squares_right);
        rotateImage(image,angle,left_eye,right_eye);

        int c = findSquares(image, squares_left,left_rect);
        int d = findSquares(image, squares_right,right_rect);

        if(c == d and c== a ) {
            sortSquares(squares_left,left_eye,LEFT);
            sortSquares(squares_right,right_eye,RIGHT);

            angle = findAngle(squares_left,squares_right);
            rotateImage(image,angle,left_eye,right_eye);
        }

    }
    drawSquares(image, left_eye);
    drawSquares(image, right_eye);


}
cv::Mat AnswerSheet::DrawChoices (int referenceEye_,int distanceWidth_,int distanceHeight_,int choiceWidth_,int choiceHeight_,
                                  int choiceNumber_, int distanceChoiceChoice_, int numberOfQuestions_, int columnDistance_,
                                  int barcodeX_,int barcodeY_,int barcodeWidth_,int barcodeHeight_,bool row_question_order_,int rowDistance_,
                                  bool has_code_,int code_refrenceEye_,int code_numCode_,int code_distanceWidth_,int code_distanceHeight_, int code_distanceChoice_) {
    referenceEye =  referenceEye_;
    distanceWidth = distanceWidth_ ;
    distanceHeight = distanceHeight_ ;
    choiceWidth = choiceWidth_;
    choiceHeight = choiceHeight_;
    choiceNumber = choiceNumber_;
    distanceChoiceChoice = distanceChoiceChoice_;
    numberOfQuestions = numberOfQuestions_ ;
    columnDistance = columnDistance_;
    rowDistance = rowDistance_ ;
    rowQuestionOrder = row_question_order_ ;

    barcodeX =barcodeX_ ;
    barcodeY = barcodeY_;
    barcodeWidth = barcodeWidth_ ;
    barcodeHeight =barcodeHeight_;

    has_code = has_code_;
    code_refrenceEye = code_refrenceEye_ ;
    code_numCode = code_numCode_;
    code_distanceWidth = code_distanceWidth_;
    code_distanceHeight = code_distanceHeight_;
    code_distanceChoice = code_distanceChoice_ ;

    cv::Mat temp ;
    image.copyTo(temp);

    if (has_code) {
        for (int choice = 0; choice < code_numCode; ++choice) {
            cv::Rect c1 = cv::Rect(left_eye[code_refrenceEye].x+ code_distanceWidth+(choice * code_distanceChoice),
                                   ((left_eye[code_refrenceEye].y+code_distanceHeight)+(right_eye[code_refrenceEye].y+code_distanceHeight))/2  ,
                                   choiceWidth,choiceHeight) ;
            cv::rectangle(temp,c1,cv::Scalar(255,0,0),2);
        }
    }


    int ques_counter = 0 ;
    uint row = 0; uint column=0; uint groupColumnDistance = 0 ;



    while (ques_counter< numberOfQuestions) {
        if(rowQuestionOrder) {
            for (int choice = 0; choice < choiceNumber; ++choice) {
                cv::Rect c1 = cv::Rect(left_eye[row+referenceEye].x+ distanceWidth+(choice * distanceChoiceChoice)+(column*columnDistance),
                        ((left_eye[row+referenceEye].y+distanceHeight)+(right_eye[row+referenceEye].y+distanceHeight))/2  ,
                        choiceWidth,choiceHeight) ;
                cv::rectangle(temp,c1,cv::Scalar(255,0,0),2);
            }
            ques_counter++ ;
            row++ ;
            if(row+referenceEye == left_eye.size() ) {
                column++ ;
                row = 0;

            }
        }
        else {
            for (int choice = 0; choice < choiceNumber; ++choice) {
                cv::Rect c1 = cv::Rect(( left_eye[row+referenceEye].x + distanceWidth) + (column*rowDistance)+groupColumnDistance,
                        (( left_eye[row+referenceEye+choice].y + distanceHeight)+ (right_eye[row+referenceEye+choice].y+distanceHeight))/2 + (choice * distanceChoiceChoice) ,
                        choiceWidth,choiceHeight) ;
                cv::rectangle(temp,c1,cv::Scalar(255,0,0),2);
            }
            ques_counter++ ;
            column++ ;
            if(column%5==0){
                groupColumnDistance+= columnDistance;
            }

            if(column == 25 ) {
                groupColumnDistance=0 ;
                row+=choiceNumber ;
                column = 0;

            }

        }

    }
    cv::rectangle(temp,cv::Rect(barcodeX,barcodeY,barcodeWidth,barcodeHeight),cv::Scalar(255,0,0),2);


    return temp ;

}

void AnswerSheet::RemoveColorsDialog() {
    cv::namedWindow( wndname, 1 );
    cv::setMouseCallback(wndname,mouseHandler) ;
    cv::imshow(wndname,image) ;
    try {
        char c = (char)cv::waitKey();
        if( c == 27 ) {
            cv::destroyAllWindows();
            return ;

        }
        if(c== 'c') {
            cv::destroyAllWindows();
            return ;
        }
    }
    catch (const std::exception e){

    }
}

void AnswerSheet::omitColors(cv::Mat& img) {

    cv::Mat mask;
    for (uint var = 0; var < omit_colors.size(); ++var) {
        omit_colors[var][0];

        cv::inRange(img,cv::Scalar(omit_colors[var][0][0],omit_colors[var][0][1],omit_colors[var][0][2]),
                cv::Scalar(omit_colors[var][1][0],omit_colors[var][1][1],omit_colors[var][1][2]), mask);
        img.setTo(cv::Scalar(255,255,255),mask);

    }
}

void omitColors(cv::Mat& img) {

    cv::Mat mask;
    for (uint var = 0; var < omit_colors.size(); ++var) {
        omit_colors[var][0];

        cv::inRange(img,cv::Scalar(omit_colors[var][0][0],omit_colors[var][0][1],omit_colors[var][0][2]),
                cv::Scalar(omit_colors[var][1][0],omit_colors[var][1][1],omit_colors[var][1][2]), mask);
        img.setTo(cv::Scalar(255,255,255),mask);

    }
}
void mouseHandler (int event, int x, int y, int flags, void *userdata){
    if(event==cv::EVENT_FLAG_LBUTTON) {
        std::vector<cv::Vec3b> omit_color ;
        cv::Vec3b color = image.at<cv::Vec3b>(y,x) ;

        if (color!=cv::Vec3b(255,255,255)) {
            //            std::cout<< color << std::endl ;
            omit_color.push_back(cv::Vec3b(
                                     int(color[0])-color_omit_rang < 0 ? 0 : int(color[0])-color_omit_rang,
                    int(color[1])-color_omit_rang < 0 ? 0 : int(color[1])-color_omit_rang,
                    int(color[2])-color_omit_rang < 0 ? 0 : int(color[2])-color_omit_rang));
            omit_color.push_back(cv::Vec3b(
                                     int(color[0])+color_omit_rang > 255 ? 255 : int(color[0])+color_omit_rang,
                    int(color[1])+color_omit_rang > 255 ? 255 : int(color[1])+color_omit_rang,
                    int(color[2])+color_omit_rang > 255 ? 255 : int(color[2])+color_omit_rang));
            omit_colors.push_back(omit_color);
            omitColors(image) ;
            cv::imshow(wndname,image);
        }
    }
}

double angle( cv::Point pt1, cv::Point  pt2, cv::Point  pt0 )
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

int AnswerSheet::findSquares( const cv::Mat& image, std::vector<std::vector<cv::Point> >& squares,cv::Rect area )
{
    squares.clear();

    cv::Mat thr ;
    cv::Mat roi_image= image(area) ;
    cv::cvtColor(roi_image,thr,cv::COLOR_BGR2GRAY) ;

    //add white padding to eyes areas
    thr(cv::Rect(0,0,thr.cols,40)) = cv::Scalar(255) ;
    thr(cv::Rect(0,thr.rows-40,roi_image.cols,40)) = cv::Scalar(255) ;

    if(area.x<600)
        thr(cv::Rect(0,0,20,thr.rows)) = cv::Scalar(255) ;
    else
        thr(cv::Rect(thr.cols-20,0,20,thr.rows)) = cv::Scalar(255) ;


    cv::threshold(thr,thr,eyes_darkness_threshold,255,cv::THRESH_BINARY_INV) ;

    cv::erode(thr, thr, elementErode);

    cv::dilate(thr, thr, elementDilate);

    cv::vector<cv::vector<cv::Point> > contours;
    cv::findContours(thr, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    cv::vector<cv::Point> approx;

    //find squre shapes
    for( size_t i = 0; i < contours.size(); i++ )
    {
        cv::approxPolyDP(cv::Mat(contours[i])   , approx, arcLength(cv::Mat(contours[i]), true)*0.10, true);
        if( approx.size() == 4 &&
                fabs(contourArea(cv::Mat(approx))) > min_squre_size && fabs(contourArea(cv::Mat(approx))) < max_squre_size &&
                cv::isContourConvex(cv::Mat(approx)) )
        {
            double maxCosine = 0;

            for( int j = 2; j < 5; j++ )
            {
                double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                maxCosine = MAX(maxCosine, cosine);
            }

            if( maxCosine < 0.3 ){
                squares.push_back(approx);
            }
        }

    }
    return(squares.size());
}

void AnswerSheet::drawSquares( cv::Mat& image, std::vector<cv::Rect> rect_vector , cv::Scalar color )
{
    for( uint i = 0; i < rect_vector.size(); i++ )
    {
        cv::rectangle(image,rect_vector[i],color,2);
        cv::putText(image, std::to_string(i), cv::Point(rect_vector[i].x,rect_vector[i].y), CV_FONT_HERSHEY_SIMPLEX, 1,
                    cv::Scalar(0,0,255), 2, 1);

    }

}

void AnswerSheet::rotateImage(cv::Mat& image,double angle, std::vector<cv::Rect>& eyes_left , std::vector<cv::Rect>& eyes_right ) {
    cv::Point2f center(image.cols/2.0, image.rows/2.0);
    cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1.0);
    cv::warpAffine(image, image, rot, resize_size ,cv::INTER_CUBIC,cv::BORDER_CONSTANT,
                   cv::Scalar(255, 255, 255));


    for (uint idx = 0; idx < eyes_left.size(); ++idx) {
        rotateRect(eyes_left[idx],rot) ;
        rotateRect(eyes_right[idx],rot) ;

    }

}

void AnswerSheet::rotateImage(cv::Mat& image,double angle) {
    cv::Point2f center(image.cols/2.0, image.rows/2.0);
    cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1.0);
    cv::warpAffine(image, image, rot, resize_size ,cv::INTER_CUBIC,cv::BORDER_CONSTANT,
                   cv::Scalar(255, 255, 255));

}
// sort order of paper eyes
void AnswerSheet::sortSquares( const std::vector<std::vector<cv::Point> >& squares ,
                               std::vector<cv::Rect>& rect_vector , eye_pose pose )
{
    rect_vector.clear();
    for( int i = squares.size()-1; i >= 0; --i )
    {
        cv::Rect rect_area ;
        int min_x =  squares[i][0].x ;
        int max_x =  squares[i][0].x ;
        int min_y =  squares[i][0].y ;
        int max_y =  squares[i][0].y ;
        for (uint point_idx = 0; point_idx < squares[i].size(); ++point_idx) {
            min_x = min_x > squares[i][point_idx].x ? squares[i][point_idx].x : min_x ;
            max_x = max_x < squares[i][point_idx].x ? squares[i][point_idx].x : max_x ;
            min_y = min_y > squares[i][point_idx].y ? squares[i][point_idx].y : min_y ;
            max_y = max_y < squares[i][point_idx].y ? squares[i][point_idx].y : max_y ;

        }
        if(pose==LEFT){
            rect_area = cv::Rect(min_x,min_y,(max_x - min_x),(max_y - min_y)) ;
        }
        else
        {
            rect_area = cv::Rect((image_width-pad_rect) + min_x,min_y,(max_x - min_x),(max_y - min_y)) ;
        }
        rect_vector.push_back(rect_area);
    }

}
// find angle of paper rotation
double AnswerSheet::findAngle(const std::vector<std::vector<cv::Point> >& squares_left , const std::vector<std::vector<cv::Point> >& squares_right)
{
    if(squares_left.size()==0 )
        return 0 ;
    cv::Point p_1 = squares_left[eye_number -1][0] ;
    cv::Point p_2 = squares_right[eye_number -1][0] ;
    double ang1 = atan (double(p_2.y-p_1.y)/(((p_2.x+image_width-pad_rect)/2)-p_1.x) * 180 / PI) ;
    p_1 = squares_left[0][0] ;
    p_2 = squares_right[0][0] ;
    double ang2 = atan (double(p_2.y-p_1.y)/(((p_2.x+image_width-pad_rect)/2)-p_1.x) * 180 / PI) ;

    return ((ang1+ang2)/2);

}
// rotate positon of detected eyes
void AnswerSheet::rotateRect(cv::Rect& rect, const cv::Mat & rot)
{
    std::vector<cv::Point2f> rot_roi_points;
    std::vector<cv::Point2f> roi_points = {
        {(float)rect.x,              (float)rect.y},
        {(float) (rect.x + rect.width), (float) (rect.y + rect.height)}
    };
    transform(roi_points, rot_roi_points, rot);
    cv::Rect result= cv::Rect((int) round(rot_roi_points[0].x),
            (int) round(rot_roi_points[0].y),
            (int) round(rot_roi_points[1].x-rot_roi_points[0].x),
            (int) round(rot_roi_points[1].y-rot_roi_points[0].y)) ;
    rect = result ;

}

cv::Mat AnswerSheet::ProcessImage (cv::String imagePath,QString table_name,std::string out_path_orginal,std::string out_path_processd,std::string out_path_error,int thread_no, bool remove_processed_file){
    const cv::Mat img_process = cv::imread(imagePath);
    if(img_process.empty())
    {
        std::string file_name = out_path_error+ currentDateTime()+"_"+std::to_string(thread_no) + ".jpg";
        QFile::copy(QString::fromStdString(imagePath), QString::fromStdString(file_name));
        std::cout << "can not open " << imagePath << "\n";
        return empty;
    }

    AnswerSheet::OMRResult OMRresult ;

    cv::Mat img_resize,img_resized_omitcolors,img_resize_out ;
    double resize_factor = 1 ;

    if (img_process.cols!=resize_size.width || img_process.rows !=resize_size.height ) {
        resize_factor =  (double) img_process.cols /  (double)  resize_size.width;
        cv::resize(img_process,img_resize,resize_size) ;
    }
    else {
        img_process.copyTo(img_resize);
    }
    img_resize.copyTo(img_resized_omitcolors);

    omitColors(img_resized_omitcolors);

    cv::Rect left_rect = cv::Rect(0,0,pad_rect,img_resize.rows);
    cv::Rect right_rect = cv::Rect(img_resize.cols-pad_rect,0,pad_rect,img_resize.rows);

    std::vector<cv::Rect> left_eye_sheet,right_eye_sheet;

    cv::vector<cv::vector<cv::Point>> squares_left,squares_right;
    int a = findSquares(img_resized_omitcolors, squares_left,left_rect);
    int b = findSquares(img_resized_omitcolors, squares_right,right_rect);

    if (a==eye_number && b==eye_number){
        sortSquares(squares_left,left_eye_sheet,LEFT);
        sortSquares(squares_right,right_eye_sheet,RIGHT);
        double angle = findAngle(squares_left,squares_right);

        rotateImage(img_resize,angle,left_eye_sheet,right_eye_sheet);
        rotateImage(img_resized_omitcolors,angle);

        double factor1 = (double)(right_eye[0].x - left_eye[0].x ) / (double)(right_eye_sheet[0].x - left_eye_sheet[0].x);
        double factor2 = (double)(right_eye[eye_number-1].y  - right_eye[0].y ) / (double)(right_eye_sheet[eye_number-1].y - right_eye_sheet[0].y);
        cv::resize(img_resized_omitcolors,img_resized_omitcolors,cv::Size(),factor1,factor2);
        cv::resize(img_resize,img_resize,cv::Size(),factor1,factor2);


        left_rect = cv::Rect(0,0,pad_rect,img_resize.rows);
        right_rect = cv::Rect(img_resize.cols-pad_rect,0,pad_rect,img_resize.rows);

        int c = findSquares(img_resized_omitcolors, squares_left,left_rect);
        int d = findSquares(img_resized_omitcolors, squares_right,right_rect);

//                    drawSquares(img_resized_omitcolors, left_eye_sheet);
//                    drawSquares(img_resized_omitcolors, right_eye_sheet);
//        cv::imwrite("d:/testttt.jpg",img_resized_omitcolors) ;
//        std::cout << c << std::endl  ;
//        std::cout << d << std::endl  ;

        if(c == d && c == eye_number ) {
            sortSquares(squares_left,left_eye_sheet,LEFT);
            sortSquares(squares_right,right_eye_sheet,RIGHT);

            angle = findAngle(squares_left,squares_right);

            rotateImage(img_resize,angle,left_eye_sheet,right_eye_sheet);
            rotateImage(img_resized_omitcolors,angle);


//            cv::Point2f srcTri[4];
//            cv::Point2f dstTri[4];

//            /// Set your 3 points to calculate the  Affine Transform
//             srcTri[0] = cv::Point2f( left_eye[0].x,left_eye[0].y );
//             srcTri[1] = cv::Point2f( left_eye[eye_number-1].x,left_eye[eye_number-1].y  );
//             srcTri[2] = cv::Point2f( right_eye[0].x,right_eye[0].y  );
//             srcTri[3] = cv::Point2f( right_eye[eye_number-1].x,right_eye[eye_number-1].y  );

//             dstTri[0] = cv::Point2f( left_eye_sheet[0].x,left_eye_sheet[0].y );
//             dstTri[1] = cv::Point2f( left_eye_sheet[eye_number-1].x,left_eye_sheet[eye_number-1].y  );
//             dstTri[2] = cv::Point2f( right_eye_sheet[0].x,right_eye_sheet[0].y  );
//             srcTri[3] = cv::Point2f( left_eye_sheet[eye_number-1].x,right_eye_sheet[eye_number-1].y  );



//            /// Get the Affine Transform
//            cv::Mat warp_mat = getAffineTransform( srcTri, dstTri );


//            /// Apply the Affine Transform just found to the src image
//            cv::warpAffine( img_resize, img_resize, warp_mat, resize_size , cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255));
//            cv::warpAffine( img_resized_omitcolors, img_resized_omitcolors, warp_mat, resize_size );
//            cv::imwrite("d:/testttt.jpg",img_resize) ;



//            drawSquares(img_resize, left_eye_sheet);
//            drawSquares(img_resize, right_eye_sheet);

            img_resize.copyTo(img_resize_out);

            OMRresult = readChoices(img_resize_out,img_resized_omitcolors,left_eye_sheet,right_eye_sheet) ;

        }
        else {
            std::string file_name ;
            file_name = out_path_error+ currentDateTime()+"_"+ std::to_string(thread_no) + ".jpg";
            QFile::copy(QString::fromStdString(imagePath), QString::fromStdString(file_name));
//            cv::imwrite(file_name,img_process) ;
            return  empty;
        }

        //read barcode
        std::string barcode = " " ,fileName;
        cv::Mat gray_barcode,color_barcode,bin_barcode;

        //        cv::Mat(img_process(cv::Rect(barcodeX*resize_factor,barcodeY*resize_factor,barcodeWidth*resize_factor,barcodeHeight*resize_factor))).copyTo(gray_barcode);
        cv::Mat(img_resized_omitcolors(cv::Rect(barcodeX,barcodeY,barcodeWidth,barcodeHeight))).copyTo(color_barcode);
        cv::cvtColor(color_barcode,gray_barcode,cv::COLOR_BGR2GRAY) ;
        cv::threshold(gray_barcode,bin_barcode,120,255,cv::THRESH_BINARY);
//        cv::imshow("sa",gray_barcode) ;
//        cv::waitKey(0);

        std::unique_lock<std::mutex> lck (mtx,std::defer_lock);
        lck.lock();
        QZXing decoder;
        decoder.setDecoder(QZXing::DecoderFormat_QR_CODE  );
        barcode = decoder.decodeImage(ASM::cvMatToQImage(color_barcode)).toStdString();
        if(barcode.size() == 0)
             barcode = decoder.decodeImage(ASM::cvMatToQImage(gray_barcode)).toStdString();
        if(barcode.size() == 0)
             barcode = decoder.decodeImage(ASM::cvMatToQImage(bin_barcode)).toStdString();
        lck.unlock();

        if (barcode.size() != 0) {
            fileName = barcode+ ".jpg";
            cv::putText(img_resize_out, barcode, cv::Point(barcodeX,(barcodeY+barcodeY+barcodeHeight)/2), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar( 255, 0, 0 ),2);
        }
        else {
            fileName= currentDateTime() +"_"+ std::to_string(thread_no) + ".jpg" ;
        }

        //save orginal and processed file
        std::string file_name_org , file_name_proc ;

        file_name_proc = out_path_processd + fileName ;
        cv::imwrite(file_name_proc,img_resize_out) ;

        file_name_org = out_path_orginal + fileName ;
        cv::imwrite(file_name_org,img_resize) ;

        //insert result to table
        QSqlQuery query;
        QString querytxt = "INSERT INTO " + table_name + " ( code, answers,orginalFilePath,processedFilePath,date,averageBlackness,choicesArea) "
                                                         "VALUES ( :code, :answers , :orginalFilePath ,:processedFilePath,:date,:averageBlackness,:choicesArea)" ;
        query.prepare(querytxt);
        query.bindValue(":code", barcode.size() == 0 ? " " : QString::fromStdString( barcode));
        query.bindValue(":answers", QString::fromStdString(OMRresult.choices));
        query.bindValue(":orginalFilePath", QString::fromStdString(file_name_org));
        query.bindValue(":processedFilePath", QString::fromStdString(file_name_proc));
        query.bindValue(":date", QString::fromStdString(currentDateTime()));
        query.bindValue(":averageBlackness",OMRresult.blackness);
        query.bindValue(":choicesArea",OMRresult.choiceAreas);
        query.exec();
        if(remove_processed_file) {
            QFile::remove(QString::fromStdString(imagePath)) ;
        }

        return img_resize_out ;

    }
    else {
        std::string file_name ;
        file_name = out_path_error+ currentDateTime()+"_"+std::to_string(thread_no) + ".jpg";
        QFile::copy(QString::fromStdString(imagePath), QString::fromStdString(file_name));
//        cv::imwrite(file_name,img_process) ;
        return  empty;

    }
}

AnswerSheet::OMRResult AnswerSheet::readChoices(cv::Mat& img_org ,cv::Mat& img ,std::vector<cv::Rect> left_eye_sheet,std::vector<cv::Rect> right_eye_sheet){
    cv::Mat threshold,img_process ;
    img_org.copyTo(img_process);
    int ques_counter = 0 ;  uint row = 0; uint column=0;
    std::string answer_string = "" ;
    cv::cvtColor(img,threshold,cv::COLOR_RGB2GRAY) ;
    cv::bitwise_not(threshold,threshold) ;
    //    cv::imshow("as",threshold) ;
    //    cv::waitKey(0) ;
    //    cv::threshold(threshold,threshold,200,255,cv::THRESH_BINARY_INV) ;
    std::vector<cv::vector<cv::Rect>> choicesRect ;

    // get min a max balckness of choiced squres
    int min = 1000 ;
    int max = max_cell ;
    int sum_max_choice_difference = 0;
    int sum_min_choice_difference = 0;
    int avg_max_choice_difference = 0;
    int avg_min_choice_difference = 0;
    bool firstCheck = true ;
    uint groupColumnDistance = 0 ;

    for (ques_counter = 0 ; ques_counter < numberOfQuestions ; ques_counter++) {
        std::vector<cv::Rect> temp ;
        int choice_value [choiceNumber] ;
        if(rowQuestionOrder) {
            for (int choice = 0; choice < choiceNumber;) {
                cv::Rect c1 = cv::Rect(left_eye_sheet[row+referenceEye].x+ distanceWidth+(choice * distanceChoiceChoice)+(column*columnDistance),
                        ((left_eye_sheet[row+referenceEye].y+distanceHeight)+(right_eye_sheet[row+referenceEye].y+distanceHeight))/2  ,
                        choiceWidth,choiceHeight) ;


                temp.push_back(c1);
                cv::Mat choice_roi = threshold(c1);
                choice_value[choice] = cv::sum(choice_roi)[0]/c1.area() ;

                if (cv::sum(choice_roi)[0]/c1.area() > max ) {
                    if (firstCheck) {
                        ques_counter = 0 ;  row = 0; column=0; choice = 0 ;
                        max = cv::sum(choice_roi)[0]/c1.area() ;
                        choicesRect.clear();
                        temp.clear();
                        firstCheck=false ;
                        continue ;
                    }
                    max = cv::sum(choice_roi)[0]/c1.area() ;
                }
                if (cv::sum(choice_roi)[0]/c1.area() < min ) min = cv::sum(choice_roi)[0]/c1.area() ;
                choice++;
            }





            int* max_choice_pure = std::max_element(choice_value,choice_value+choiceNumber);
            int* min_choice_pure = std::min_element(choice_value,choice_value+choiceNumber);

            sum_max_choice_difference += *max_choice_pure;
            sum_min_choice_difference += *min_choice_pure;

            choicesRect.push_back(temp);
            row++ ;
            if(row+referenceEye == left_eye_sheet.size() ) {
                column++ ;
                row = 0;
            }
        }
        else {
            for (int choice = 0; choice < choiceNumber;) {
                cv::Rect c1 = cv::Rect(( left_eye_sheet[row+referenceEye].x + distanceWidth) + (column*rowDistance)+groupColumnDistance,
                        (( left_eye_sheet[row+referenceEye+choice].y + distanceHeight)+ (right_eye_sheet[row+referenceEye+choice].y+distanceHeight))/2 + (choice * distanceChoiceChoice) ,
                        choiceWidth,choiceHeight) ;
                temp.push_back(c1);
                cv::Mat choice_roi = threshold(c1);

                if (cv::sum(choice_roi)[0]/c1.area() > max ) {
                    if (firstCheck) {
                        ques_counter = 0 ;  row = 0; column=0; choice = 0 ; groupColumnDistance=0;
                        max = cv::sum(choice_roi)[0]/c1.area() ;
                        choicesRect.clear();
                        temp.clear();
                        firstCheck=false ;
                        continue ;
                    }
                    max = cv::sum(choice_roi)[0]/c1.area() ;
                }
                if (cv::sum(choice_roi)[0]/c1.area() < min ) min = cv::sum(choice_roi)[0]/c1.area() ;
                choice++;
            }
            choicesRect.push_back(temp);
            column++ ;
            if(column%5==0){
                groupColumnDistance+= columnDistance;
            }

            if(column == 25 ) {
                groupColumnDistance=0 ;
                row+=choiceNumber ;
                column = 0;

            }

        }

    }
//        std::cout << min << "-" << max << std::endl ;
    if(max-min < 10) { //empty page
        max =max_cell ;
    }

    //detect selected choices
    int choicesSum = 0 ;
    int choicesCounter=0;
    int darkness_threshold = thresh_choice_value;


    avg_max_choice_difference =  sum_max_choice_difference/numberOfQuestions ;
    avg_min_choice_difference =  sum_min_choice_difference/numberOfQuestions ;



    firstCheck = true ;
    row = 0 ;
    column=0 ;
    for (ques_counter = 0  ; ques_counter < numberOfQuestions ; ) {
        bool ischoiced = false ; bool twochoice = false ;
        int selected_choice = 0 ;

        int choice_value [choiceNumber] ;
        int choice_value_pure [choiceNumber] ;
        for (int choice = 0; choice < choiceNumber; ++choice) {
            cv::Rect c1 = choicesRect[ques_counter][choice] ;
            cv::Mat choice_roi = threshold(c1);
            choice_value[choice] = (((cv::sum(choice_roi)[0]/c1.area())-min)/(max-min)) * 100 ; // normalize blackness of the choices
            choice_value_pure[choice] = cv::sum(choice_roi)[0]/c1.area() ;
        }





        int* max_choice = std::max_element(choice_value,choice_value+choiceNumber);
        int* max_choice_pure = std::max_element(choice_value_pure,choice_value_pure+choiceNumber);
        bool isEmpty = true ;
        if(*max_choice>thresh_choice_value)
            isEmpty = false ;
        else
            for (int choice = 0; choice < choiceNumber; ++choice) {
//                std::cout<< ques_counter <<"-" <<  choice<< ":" << (*max_choice_pure -choice_value_pure[choice])<< std::endl ;
                 if ((*max_choice - choice_value[choice])>=thresh_choice_value || ((*max_choice_pure - choice_value_pure[choice])>=max/2))
                     isEmpty=false  ;
            }

        int max_idx = std::distance(choice_value, max_choice) ; //|| choice_value_pure[max_idx]>(max/2)
        if (!isEmpty && *max_choice_pure > (avg_max_choice_difference-(20*avg_max_choice_difference/100)) && max!=max_cell && (*max_choice>=darkness_threshold || (max -choice_value_pure[max_idx])<(max/4) || choice_value_pure[max_idx]>(max/2) )) { //choiced
 //       if (!isEmpty && max!=max_cell && (*max_choice>=darkness_threshold || choice_value_pure[max_idx]>(max/2))) { //choiced

            choicesSum+= *max_choice ;
            choicesCounter += 1 ;


            for (int choice = 0; choice < choiceNumber; ++choice) { //check duplicated choice
//                qDebug()<<ques_counter << ":" << *max_choice << "-" << choice_value[choice] ;
//                if ( choice!= max_idx && max!= 50 &&  ((*max_choice-choice_value[choice])<=(darkness_threshold/(2)) || choice_value[choice]>70 || (max - choice_value_pure[choice])<40) ) {
                    if (choice!= max_idx && max!= max_cell &&  (choice_value[choice]>=darkness_threshold+25 && (*max_choice - choice_value[choice])<=15 )) {
                    cv::rectangle(img_process,choicesRect[ques_counter][choice] ,cv::Scalar(0,0,255),2);
                    if (!twochoice) {
                        cv::rectangle(img_process,choicesRect[ques_counter][max_idx],cv::Scalar(0,0,255),2);
                        twochoice=true ;
                    }
                }
            }
            if (!twochoice) {
                selected_choice = max_idx+1 ;
                ischoiced = true ;
                cv::rectangle(img_process,choicesRect[ques_counter][max_idx] ,cv::Scalar(0,255,0),2);
            }

        }
        else //empty
            for (int choice = 0; choice < choiceNumber; ++choice){
                cv::rectangle(img_process,choicesRect[ques_counter][choice],cv::Scalar(255,0,0),2);
            }

        if (twochoice) {
            answer_string+='*';
        }
        else if (ischoiced) {
            if(rowQuestionOrder)
                answer_string+=std::to_string(selected_choice) ;
            else
                answer_string+=std::to_string(--selected_choice) ;
        }
        else {
            answer_string+=' ';
        }
        row++ ;
        if(row+referenceEye == left_eye_sheet.size() ) {
            column++ ;
            row = 0;
        }
        ques_counter++ ;
        if ( ques_counter == numberOfQuestions-1 && choicesCounter!= 0 &&  choicesSum/choicesCounter<= 90 && firstCheck) { //check the average darkness of choices
            answer_string = "" ;
            ques_counter = 0 ,row = 0,  column=0 ;
            choicesCounter = 0 ;
            choicesSum = 0 ;
            darkness_threshold = 20 ;
            firstCheck = false ;
            img_org.copyTo(img_process);
        }
    }

    //detect code number
    std::vector<cv::Rect> choicesRectCode ;
    if (has_code) {
//        answer_string+=',';
        int choice_value [code_numCode] ;
        int choice_value_pure [code_numCode] ;
        bool ischoiced = false ; bool twochoice = false ;
        int selected_choice = 0 ;

        for (int choice = 0; choice < code_numCode; ++choice) {
            cv::Rect c1 = cv::Rect(left_eye_sheet[code_refrenceEye].x+ code_distanceWidth+(choice * code_distanceChoice),
                                   ((left_eye_sheet[code_refrenceEye].y+code_distanceHeight)+(right_eye_sheet[code_refrenceEye].y+code_distanceHeight))/2  ,
                                   choiceWidth,choiceHeight) ;
            choicesRectCode.push_back(c1);
            cv::Mat choice_roi = threshold(c1);
            choice_value[choice] = (((cv::sum(choice_roi)[0]/c1.area())-min)/(max-min)) * 100 ; // normalize blackness of the choices
            choice_value_pure[choice] = cv::sum(choice_roi)[0]/c1.area() ;
        }
        int* max_choice = std::max_element(choice_value,choice_value+code_numCode);
        int max_idx = std::distance(choice_value, max_choice) ; //|| choice_value_pure[max_idx]>(max/2)
        if (max!=max_cell && (*max_choice>=darkness_threshold || (max -choice_value_pure[max_idx])<(max/4) || choice_value_pure[max_idx]>(max/2) )) { //choiced

            choicesSum+= *max_choice ;
            choicesCounter += 1 ;
            for (int choice = 0; choice < code_numCode; ++choice) { //check duplicated choice
                if ( choice!= max_idx && max!= max_cell &&  ((*max_choice-choice_value[choice])<=(darkness_threshold/(2)) || choice_value[choice]>70 || (max - choice_value_pure[choice])<40) ) {
                    cv::rectangle(img_process,choicesRectCode[choice] ,cv::Scalar(0,0,255),2);
                    if (!twochoice) {
                        cv::rectangle(img_process,choicesRectCode[max_idx],cv::Scalar(0,0,255),2);
                        twochoice=true ;
                    }
                }
            }
            if (!twochoice) {
                selected_choice = max_idx+1 ;
                ischoiced = true ;
                cv::rectangle(img_process,choicesRectCode[max_idx] ,cv::Scalar(0,255,0),2);
            }

        }
        else //empty
            for (int choice = 0; choice < code_numCode; ++choice)
                cv::rectangle(img_process,choicesRectCode[choice],cv::Scalar(255,0,0),2);

        if (twochoice) {
            answer_string+='*';
        }
        else if (ischoiced) {
            if(rowQuestionOrder)
                answer_string+=std::to_string(selected_choice) ;
            else
                answer_string+=std::to_string(--selected_choice) ;
        }
        else {
            answer_string+=' ';
        }

    }
    //    if (choicesCounter!= 0 )
    //        std::cout << choicesSum/choicesCounter << std::endl ;
    img_process.copyTo(img_org);
    QString rectString = "" ;
    for (ques_counter = 0 ; ques_counter < numberOfQuestions ; ques_counter++) {
        for (int choice = 0 ; choice < choiceNumber; choice++) {
            cv::Rect temp = choicesRect[ques_counter][choice] ;
            rectString += QString::number(temp.x) + "," ;
            rectString += QString::number(temp.y) + "," ;
            rectString += QString::number(temp.width) + "," ;
            rectString += QString::number(temp.height) ;
            if(choice!=choiceNumber-1)
                rectString +="-";
        }
        if (ques_counter!=numberOfQuestions-1)
            rectString +="*";
    }
    if (has_code) {
        rectString +="*";
        for (int choice = 0; choice < code_numCode; ++choice) {
            cv::Rect temp = choicesRectCode[choice] ;
            rectString += QString::number(temp.x) + "," ;
            rectString += QString::number(temp.y) + "," ;
            rectString += QString::number(temp.width) + "," ;
            rectString += QString::number(temp.height) ;
            if(choice!=code_numCode-1)
                rectString +="-";
        }
    }


    AnswerSheet::OMRResult result= {answer_string,choicesCounter!= 0 ? choicesSum/choicesCounter : 0,rectString} ;
    return result;


}
