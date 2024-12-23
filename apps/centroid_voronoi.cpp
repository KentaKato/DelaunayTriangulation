#include "DelaunayTriangulation/delaunay_triangulation.hpp"
#include "DelaunayTriangulation/delaunay_triangulation_drawer.hpp"
#include "DelaunayTriangulation/voronoi_diagram.hpp"

#include <cstdlib>
#include <random>

namespace delaunay_triangulation
{

void addRandomVertices(
    DelaunayTriangulation &delaunay,
    const int image_width,
    const int image_height,
    const size_t num_vertices)
{
    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::uniform_real_distribution<> width_dist(0, image_width);
    std::uniform_real_distribution<> height_dist(0, image_height);
    for (size_t i = 0; i < num_vertices; ++i)
    {
        Vertex v(width_dist(engine), height_dist(engine));
        if (!delaunay.hasVertex(v))
        {
            delaunay.addVertex(v);
        }
        else {
            --i;
        }
    }
}

class CentroidVoronoiDiagram
{

public:
    explicit CentroidVoronoiDiagram(
        const size_t num_vertices,
        const int image_width,
        const int image_height,
        const size_t pixel_step_size_for_centroid_calc)
        : num_vertices_(num_vertices),
          image_width_(image_width),
          image_height_(image_height),
          delaunay_()
    {
        img_ = cv::Mat(image_height, image_width, CV_8UC3, bg_color_);
        addRandomVertices(delaunay_, image_width_, image_height_, num_vertices_);

        const auto& step = pixel_step_size_for_centroid_calc;
        bool increment = true;
        for (int x = 0; x < image_width; x += step)
        {
            // NOTE: Alternating the increment and decrement of y to give a better seed to findNearestVertex()
            if(increment)
            {
                for (int y = 0; y < image_height; y += step)
                {
                    pixel_weight_[Point{static_cast<double>(x), static_cast<double>(y)}] = 1.0;
                }
            }
            else
            {
                for (int y = image_height -1 ; y >= 0; y -= step)
                {
                    pixel_weight_[Point{static_cast<double>(x), static_cast<double>(y)}] = 1.0;
                }
            }
            increment = !increment;
        }
    }

    void run()
    {
        bool first_time = true;
        while (true)
        {
            // Create Delaunay triangles
            delaunay_.createDelaunayTriangles();

            // Create Voronoi cells
            const auto voronoi_cells = VoronoiDiagram::create(delaunay_.getAllTriangles());

            /// Compute the centroid of each Voronoi cell.
            std::unordered_map<Site, Centroid> voronoi_centroids;
            VoronoiDiagram::computeVoronoiCentroids(
                delaunay_,
                pixel_weight_,
                voronoi_centroids);

            /// Draw
            img_.setTo(bg_color_);
            VoronoiDiagram::draw( img_, voronoi_cells, voronoi_edge_color_);

            for (const auto & [site, centroid] : voronoi_centroids)
            {
                centroid.draw(img_, false, centroid_color_);
                site.draw(img_, false);
            }

            cv::imshow("Centroid Voronoi Diagram", img_);
            if (first_time)
            {
                first_time = false;
                cv::waitKey(1000);
            }
            else {
                cv::waitKey(20);
            }

            /// Update the vertices positions
            delaunay_.clear();
            delaunay_.reserveVerticesVector(num_vertices_);
            for (const auto & [_, centroid] : voronoi_centroids)
            {
                delaunay_.addVertex(centroid);
            }
        }
    }

private:
    const size_t num_vertices_;
    const cv::Scalar bg_color_{160, 160, 160};
    const cv::Scalar centroid_color_{100, 255, 0};
    const cv::Scalar voronoi_edge_color_{240, 240, 240};

    const int image_width_, image_height_;

    DelaunayTriangulation delaunay_;

    cv::Mat img_;

    /// pixel_belonging_cells_[p] is the site of the Voronoi cell that contains the point p.
    std::unordered_map<Point, Site> pixel_belonging_cells_;

    /// The weight of each point used for centroid calculation.
    std::unordered_map<Point, double> pixel_weight_;

};

} // namespace delaunay_triangulation


int main()
{
    delaunay_triangulation::CentroidVoronoiDiagram centroid_voronoi(
        50,
        500,
        500,
        4);
    centroid_voronoi.run();
    return EXIT_SUCCESS;
}

