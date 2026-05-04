#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/opencv.hpp>

class RedCubeFollower : public rclcpp::Node
{
public:
  RedCubeFollower() : Node("red_cube_follower_node")
  {
    // Объявление параметров для настройки скоростей
    this->declare_parameter("linear_speed", 0.2);
    this->declare_parameter("kp_angular", 1.5);

    linear_speed_ = this->get_parameter("linear_speed").as_double();
    kp_angular_ = this->get_parameter("kp_angular").as_double();

    // Подписка на изображение с камеры
    subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
        "/camera/image_raw", 10,
        std::bind(&RedCubeFollower::image_callback, this, std::placeholders::_1));

    // Публикация команд движения
    cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);

    RCLCPP_INFO(this->get_logger(), "RedCubeFollower node started");
  }

private:
  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg)
  {
    // Преобразование ROS-изображения в cv::Mat (как в color-follower-ros2)
    cv_bridge::CvImagePtr cv_ptr;
    try
    {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception &e)
    {
      RCLCPP_ERROR(this->get_logger(), "cv_bridge error: %s", e.what());
      return;
    }

    // Переводим в HSV
    cv::Mat hsv;
    cv::cvtColor(cv_ptr->image, hsv, cv::COLOR_BGR2HSV);

    // Создаём маску для красного цвета (два диапазона HSV)
    cv::Mat mask;
    cv::inRange(hsv, cv::Scalar(0, 100, 100), cv::Scalar(10, 255, 255), mask);
    cv::Mat mask2;
    cv::inRange(hsv, cv::Scalar(170, 100, 100), cv::Scalar(180, 255, 255), mask2);
    cv::bitwise_or(mask, mask2, mask);

    // Морфологическая очистка маски (опционально, как в оригинале)
    cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
    cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);

    // Поиск контуров
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    geometry_msgs::msg::Twist cmd;

    if (contours.empty())
    {
      // Куб не виден — поворачиваем на месте для поиска
      cmd.linear.x = 0.0;
      cmd.angular.z = 0.5;
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                           "No red cube found, searching...");
    }
    else
    {
      // Выбираем самый большой контур (предполагаем, что это куб)
      double max_area = 0.0;
      int max_idx = 0;
      for (size_t i = 0; i < contours.size(); i++)
      {
        double area = cv::contourArea(contours[i]);
        if (area > max_area)
        {
          max_area = area;
          max_idx = i;
        }
      }

      if (max_area < 500)
      {
        // Слишком маленький контур — продолжаем поиск
        cmd.linear.x = 0.0;
        cmd.angular.z = 0.5;
        RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                             "Too small area: %.0f, searching...", max_area);
      }
      else
      {
        // Определяем центр контура
        cv::Moments m = cv::moments(contours[max_idx]);
        if (m.m00 != 0)
        {
          int cx = static_cast<int>(m.m10 / m.m00);
          int img_w = cv_ptr->image.cols;

          // Вычисляем ошибку положения куба относительно центра кадра
          double error = (cx - img_w / 2.0) / (img_w / 2.0);

          // П-регулятор для поворота к кубу
          cmd.linear.x = linear_speed_;
          cmd.angular.z = -kp_angular_ * error;

          // Если куб достаточно близко (занимает > 80% ширины кадра), останавливаемся
          cv::Rect bbox = cv::boundingRect(contours[max_idx]);
          if (bbox.width > img_w * 0.8)
          {
            cmd.linear.x = 0.0;
            cmd.angular.z = 0.0;
            RCLCPP_INFO(this->get_logger(), "Arrived at red cube!");
          }
          else
          {
            RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 500,
                                 "Tracking red cube: area=%.0f, error=%.2f", max_area, error);
          }
        }
      }
    }

    cmd_pub_->publish(cmd);
  }

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
  double linear_speed_;
  double kp_angular_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<RedCubeFollower>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}