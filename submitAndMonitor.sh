#!/bin/bash

# Make exectuable
make clean && make
if [ $? -ne 0 ]; then
    echo "Error: make failed, exiting."
    exit 1
fi

# Submit the job and capture the job ID
job_output=$(sbatch job.sh)
job_id=$(echo "$job_output" | awk '{print $4}')
echo "Submitted job ${job_id}"

# Function to check job status
check_job_status() {
    job_info=$(scontrol show job "$job_id")
    job_state=$(echo "$job_info" | grep "JobState" | awk '{print $1}')
}

cancel_job() {
    scancel "$job_id"
}

# Wait for the job to complete
pending=false
running=false
while true; do
    check_job_status
    if [ "$job_state" == "JobState=COMPLETED" ]; then
        echo "Job completed!"
        slurm_err="slurm-${job_id}.out"
        if [ -s "$slurm_err" ]; then
            echo "Slurm error file not empty. Printing contents:"
            cat "$slurm_err"
        fi
        break
    elif [ "$job_state" == "JobState=CANCELLED" ] || [ "$job_state" == "JobState=FAILED" ]; then
        scontrol show job $job_id
        echo "Job failed or was cancelled."
        break
    elif [ "$job_state" == "JobState=TIMEOUT" ]; then
        echo "Job timed out."
        break
    elif [ "$job_state" == "JobState=PENDING" ] && ! $pending; then
        echo "Job pending..."
        pending=true
    elif [ "$job_state" == "JobState=RUNNING" ] && ! $running; then
        echo "Job running..."
        running=true
    else
        sleep 1  # Sleep for 1 second before checking again
    fi

    read -t 1 -n 1 input
    if [[ "$input" == "c" ]]; then  # cancel job when user types 'c'
        cancel_job
    fi
done