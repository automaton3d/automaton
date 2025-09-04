/*
 * initSim.cpp
 *
 * Gather all initialization routines.
 */

#include "simulation.h"

namespace automaton
{
  using namespace std;

  const double epsilon = 1e-9;

  // Global variables for lattice (assuming they're vectors, not functions)
  extern Cell lattice_curr[EL][EL][EL][W_DIM];
  extern Cell lattice_draft[EL][EL][EL][W_DIM];

  unsigned momenta[W_DIM][3];
  /*
   * Generate momentum vectors.
   */
  void initMomentum()
  {
    const int r_max = CENTER;
    const int n_steps = CENTER;
    const double step_size = 1.0 / n_steps;
    // Get the center cell in the first layer of the lattice
    Cell& center_cell = lattice_curr[CENTER][CENTER][CENTER][0];
    unsigned w = 0; // Counter for the number of spins initialized
    // Iterate through the lattice to initialize spin vectors
    for (unsigned x = 0; x < EL; ++x)
    {
      for (unsigned y = 0; y < EL; ++y)
      {
        for (unsigned z = 0; z < EL; ++z)
        {
          int dx = x - CENTER;
          int dy = y - CENTER;
          int dz = z - CENTER;
          int distance_squared = dx * dx + dy * dy + dz * dz;
          // Assign spin vectors based on distance thresholds
          for (int i = 0; i < n_steps; ++i)
          {
            double lower_bound = r_max - 1 + i * step_size;
            double upper_bound = r_max - 1 + (i + 1) * step_size;
            double lower_bound_sq = lower_bound * lower_bound;
            double upper_bound_sq = upper_bound * upper_bound;
            if (distance_squared >= lower_bound_sq && distance_squared < upper_bound_sq)
            {
              unsigned p[3] = { x, y, z };
  //            momenta[w] = p;
              markPoles(p, w);
              ++w;
              // Move to the next cell in a valid manner
              if (center_cell.x[0] >= 0 && center_cell.x[0] < EL &&
                  center_cell.x[1] >= 0 && center_cell.x[1] < EL &&
                  center_cell.x[2] >= 0 && center_cell.x[2] < EL)
              {
                center_cell = lattice_curr[center_cell.x[0]][center_cell.x[1]][center_cell.x[2]][0];
              }
              else
              {
                cerr << "Error: Invalid cell position encountered!" << endl;
                exit(EXIT_FAILURE);
              }
              // Stop if the required number of vector have been initialized
              if (w == W_DIM)
              {
                return;
              }
            }
          }
        }
      }
    }
    // Error handling if no spins were initialized
    if (w == 0)
    {
      cerr << "Error: No spins initialized. Check lattice dimensions and thresholds!" << endl;
      exit(EXIT_FAILURE);
    }
    puts("\tinitMomentum ok.");
  }

  /*
   * Initializes charges and affinity in the central
   * cell of every layer.
   */
  void initCharges()
  {
    for (unsigned w = 0; w < W_DIM; w++)
    {
      // Access the central cell in the current layer (w-slice)
      Cell& cell = lattice_curr[CENTER][CENTER][CENTER][w];
      char w0 = w % 2;
      char w1 = (w >> 1) % 2;
      char q = w0 ^ w1;
      // Set charge and affinity values
      cell.ch = (w % 8) | (w0 << 3) | (1 << 4) | (q << 5);
    }
    puts("initCharges ok.");
  }

  /**
   * Initialize the center cells.
   */
  void initCell0()
  {
    for (unsigned w = 0; w < W_DIM; w++)
    {
      // Access the central cell in the current layer (w-slice)
      Cell& cell = lattice_curr[CENTER][CENTER][CENTER][w];
      char w0 = w % 2;
      char w1 = (w >> 1) % 2;
      char q = w0 ^ w1;
      // Set charge and affinity values
      cell.ch = (w % 8) | (w0 << 3) | (1 << 4) | (q << 5);
    }
    puts("initCell0 ok.");
  }

  /**
   * Function to initialize the lattice.
   */
  void initGeneral()
  {
    for (unsigned w = 0; w < W_DIM; w++)
    {
      int i = 0;
      for (unsigned x = 0; x < EL; x++)
      {
        for (unsigned y = 0; y < EL; y++)
        {
          for (unsigned z = 0; z < EL; z++)
          {
            // Reference to the current cell
            Cell& cell = lattice_curr[x][y][z][w];
            // Initialize coordinates
            cell.x[0] = x;
            cell.x[1] = y;
            cell.x[2] = z;
            cell.a = w;
            //
            cell.k = 0;
            cell.t = 0;
            cell.c[0] = 0;
            cell.c[1] = 0;
            cell.c[2] = 0;
            cell.hB = 0;
            // Reference to the central cell in the current layer
            Cell& cell0 = lattice_curr[CENTER][CENTER][CENTER][w];
            // Initialize properties of the cell
            cell.ch = cell0.ch;
            // Calculate distance from center
            double dx = static_cast<double>(x - (double)CENTER);
            double dy = static_cast<double>(y - (double)CENTER);
            double dz = static_cast<double>(z - (double)CENTER);
            double d = sqrt(dx * dx + dy * dy + dz * dz);
            cell.d = static_cast<unsigned>(d);
            cell.f = 0;
            i++;
          }
        }
      }
    }
    puts("initGeneral ok.");
  }

  void initializeMask()
  {
//    int R_MAX = CENTER; // Máximo raio permitido
    /*
    for (int r = 0; r <= R_MAX; ++r)
    {
      // Gerar pontos para o raio atual
      auto points = 0;//generateUniformSpherePoints(r, L, L);
      for (const auto& point : points)
      {
        // Extrair coordenadas relativas do ponto
        int x, y, z;
        std::tie(x, y, z) = point;
        // Calcular posição absoluta com relação ao centro
        int abs_x = CENTER + x;
        int abs_y = CENTER + y;
        int abs_z = CENTER + z;
        // Verificar se está dentro dos limites
        if (abs_x >= 0 && abs_x < XXX &&
            abs_y >= 0 && abs_y < XXX &&
            abs_z >= 0 && abs_z < XXX)
        {
          // Configurar o bit mask na célula correspondente
          lattice_curr[abs_x][abs_y][abs_z][0].mask = true;
        }
      }
    }
    */
  }

  // Function to calculate and print the nearest points to the spiral curve
  void initSpiral()
  {
    // Define the theta range
    int num_points = 10 * EL;  // Number of points for the curve
    double theta[num_points];
    double r[num_points], x_curve[num_points], y_curve[num_points], z_curve[num_points];

    // Fill the theta array
    for (int i = 0; i < num_points; ++i)
    {
      theta[i] = 2 * M_PI * i / num_points;
    }
    // Calculate r, x, y, and z for the spiral curve
    for (int i = 0; i < num_points; ++i)
    {
      r[i] = (double)EL / (4 * M_PI) * theta[i];
      x_curve[i] = r[i] * cos(theta[i]) + EL / 2;
      y_curve[i] = r[i] * sin(theta[i]) + EL / 2;
      z_curve[i] = r[i] + EL / 2;  // Centered along the z-axis
    }
    // Find the grid points nearest to the curve
    int nearest_points[EL * EL * EL][3]; // Array to store the nearest points
    int nearest_point_count = 0;
    // Debug: Print first few values of x_curve, y_curve, z_curve
    printf("Debug: First few curve points:\n");
    for (int i = 0; i < 5; ++i)
    {
      printf("x: %.2f, y: %.2f, z: %.2f\n", x_curve[i], y_curve[i], z_curve[i]);
    }
    // Iterate through the points on the curve and round to nearest grid points
    for (int i = 0; i < num_points; ++i)
    {
      int x = (int)round(x_curve[i]);
      int y = (int)round(y_curve[i]);
      int z = (int)round(z_curve[i]);
      // Debug: Check the rounded values and bounds
      if (i < 5)
      {
        printf("Rounded point %d: (%d, %d, %d)\n", i, x, y, z);
      }
      // Ensure the grid point is within bounds
      if (x >= 0 && x < EL && y >= 0 && y < EL && z >= 0 && z < EL)
      {
        nearest_points[nearest_point_count][0] = x;
        nearest_points[nearest_point_count][1] = y;
        nearest_points[nearest_point_count][2] = z;
        nearest_point_count++;
      }
    }
    // Check if we found any nearest points and print them
    if (nearest_point_count > 0)
    {
        printf("Nearest points to the spiral curve:\n");
        for (int i = 0; i < nearest_point_count; ++i)
        {
            printf("Point %d: (%d, %d, %d)\n", i + 1, nearest_points[i][0], nearest_points[i][1], nearest_points[i][2]);
        }
    }
    else
    {
        printf("No nearest points found.\n");
    }
  }

  bool initSimulation(int step)
  {
	switch(step)
	{
	  case 0:
    	initCell0();
        break;
	  case 1:
        initMomentum();
        break;
      case 2:
        initSpiral();
        break;
      case 3:
        initGeneral();
        break;
      case 4:
        initializeMask();
        break;
	  case 5:
        //printConstants();
        break;
      case 11:
        std::copy(&lattice_curr[0][0][0][0],
        &lattice_curr[0][0][0][0] + BLOCK,
        &lattice_draft[0][0][0][0]);
        break;
      case 12:
        // Check consistency
        //assert(sanityTest1());
        break;
      default:
    	return true;
	}
    return false;
  }

}
