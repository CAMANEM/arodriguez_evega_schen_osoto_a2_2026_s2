#pragma once

#include <vector>

/* ===== EXECUTION MODELS ====================================================================== */
/**
 * Execution scheme measured by one metrics_interface instance.
 * Each instance is a single experimental configuration: (model + n_workers)
 * over many repeated runs of the same input.
 */
enum class execution_model {
    sequential,      /**< Baseline with no parallel execution scheme. */
    fine_grained,    /**< Fine-grained multithreading. */
    coarse_grained,  /**< Coarse-grained multithreading. */
    smt,             /**< Simultaneous multithreading (hardware SMT / software approximation). */
    cmp              /**< Chip multiprocessing (one worker per physical core). */
};


/* ===== GENERAL METRICS INTERFACE ============================================================= */


/**
 * Collects the metrics required for every problem: wall time samples,
 * speedup versus sequential, parallel efficiency, and a 95 % CI of the mean.
 *
 * One instance is one configuration (model + n_workers). Scalability comes
 * from comparing several instances, not from extra fields here.
 *
 * Problem-specific counters belong in a derived class; override record_time
 * and reset if those extras must stay in sync with the common samples.
 */
class metrics_interface {


/* ===== ATTRIBUTES ============================================================================ */


/* run_times holds every repetition of THIS configuration (typically >= 200).
 * Mean, sample standard deviation, 95 % CI and boxplots are derived from that vector.
 *
 * sequential_time is not a one-shot sequential run and not a second sample
 * list. It is the scalar baseline T_seq used by speedup() and efficiency():
 *     speedup     = sequential_time / mean(run_times)
 *     efficiency  = speedup / n_workers
 * After the sequential campaign finishes, copy its mean into every parallel
 * instance with set_sequential_time(seq.mean_time()). On the sequential
 * instance itself, set sequential_time to its own mean so speedup ~= 1.
 */
protected:
    execution_model model;          // scheme measured in run_times
    int n_workers;                  // threads or processes for this config
    double sequential_time;         // T_seq baseline (mean of sequential runs)
    std::vector<double> run_times;  // wall times (s) of this config's runs

public:

    /* ===== CONSTRUCTOR AND DESTRUCTOR ======================================================== */
    /**
     * @brief Builds an empty sample set for the given configuration.
     * @param model Execution scheme stored in this instance.
     * @param n_workers Threads or processes used by this configuration.
     */
    explicit metrics_interface(execution_model model = execution_model::sequential,
                              int n_workers = 1);

    /**
     * @brief Virtual destructor so derived metric types can be deleted via base pointer.
     */
    virtual ~metrics_interface() = default;


    /* ===== OTHER METHODS ===================================================================== */


    /**
     * @brief Appends one run's wall time to this configuration's sample list.
     * @param seconds Elapsed time of the run, in seconds.
     * @note Override in a derived class to record extra per-run data as well.
     */
    virtual void record_time(double seconds);

    /**
     * @brief Clears recorded run times so a new campaign can start.
     * @note Does not change model, n_workers, or sequential_time.
     *       Override in a derived class to also clear extra per-run data.
     */
    virtual void reset();

    /**
     * @brief Mean wall time over recorded runs.
     * @return Arithmetic mean of run_times, or 0 if there are no samples.
     */
    virtual double mean_time() const;

    /**
     * @brief Sample standard deviation of wall time.
     * @return s = sqrt(sum (x_i - mean)^2 / (n - 1)), or 0 if n < 2.
     */
    virtual double stddev_time() const;

    /**
     * @brief Speedup of this configuration versus the sequential baseline.
     * @return sequential_time / mean_time(), or 0 if either value is not positive.
     */
    virtual double speedup() const;

    /**
     * @brief Parallel efficiency of this configuration.
     * @return speedup() / n_workers, or 0 if n_workers is not positive.
     */
    virtual double efficiency() const;

    /**
     * @brief Lower bound of the 95 % confidence interval of the mean wall time.
     * @return mean_time() - 1.96 * stddev_time() / sqrt(n), or the mean if n < 2.
     */
    virtual double ci95_lower() const;

    /**
     * @brief Upper bound of the 95 % confidence interval of the mean wall time.
     * @return mean_time() + 1.96 * stddev_time() / sqrt(n), or the mean if n < 2.
     */
    virtual double ci95_upper() const;


    /* ===== GETTERS =========================================================================== */


    /**
     * @brief Execution scheme of this configuration.
     * @return Value of model.
     */
    execution_model get_model() const {return model;}

    /**
     * @brief Worker count of this configuration.
     * @return Threads or processes stored in n_workers.
     */
    int get_n_workers() const {return n_workers;}

    /**
     * @brief Number of recorded repetitions.
     * @return Size of run_times.
     */
    int get_run_count() const {return static_cast<int>(run_times.size());}

    /**
     * @brief Sequential baseline T_seq used for speedup.
     * @return sequential_time (typically the mean of the sequential campaign).
     */
    double get_sequential_time() const {return sequential_time;}

    /**
     * @brief All recorded wall times for this configuration.
     * @return Const reference to run_times, in seconds.
     */
    const std::vector<double>& get_run_times() const {return run_times;}


    /* ===== SETTERS =========================================================================== */

    
    /**
     * @brief Sets the execution scheme of this configuration.
     * @param m New model value.
     */
    void set_model(execution_model m) {model = m;}

    /**
     * @brief Sets the worker count of this configuration.
     * @param n Threads or processes (must be positive for a valid efficiency).
     */
    void set_n_workers(int n) {n_workers = n;}

    /**
     * @brief Sets the sequential baseline T_seq.
     * @param t Mean wall time of the sequential campaign, in seconds.
     */
    void set_sequential_time(double t) {sequential_time = t;}

private:
    /**
     * @brief Half-width of the 95 % CI of the mean (normal approximation).
     * @return 1.96 * stddev_time() / sqrt(n), or 0 if n < 2.
     */
    double ci95_half_width() const;
};
