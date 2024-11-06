% MATLAB script to read audio data and plot 3 spectrograms for different time intervals

% Read the audio data from the file
[ns_data, fs] = audioread("3HP-NovoEngine.mp3");

% plot data 
%plot(ns_data);

% Create a time vector based on the sampling frequency and data length
t = (0:length(ns_data)-1) / fs;

% Define window parameters
windowLength = 128; % Length of the window
window = hamming(windowLength);

% Define the time intervals for analysis (in seconds)
time_intervals = [0 1.5; 1.5 3; 3 4.5];

numSamples = length(ns_data);

% Plot 3 spectrograms for the time intervals
figure;
for i = 1:size(time_intervals, 1)
    % Extract the portion of the signal for the current time interval
    startTime = time_intervals(i, 1);
    endTime = time_intervals(i, 2);
    
    % Convert time to sample indices
    startIdx = round(startTime * fs) + 1;
    endIdx = round(endTime * fs);
    
    % Ensure the indices are within bounds
    if startIdx < 1
        startIdx = 1;
    end
    if endIdx > numSamples
        endIdx = numSamples;
    end
    
    % Extract the segment
    segment = ns_data(startIdx:endIdx);
    
    % Time vector for the segment
    segmentTime = t(startIdx:endIdx);
    
    % Plot the spectrogram
    subplot(3, 1, i); % Create a subplot for each spectrogram
    spectrogram(segment, window, windowLength/2, 1024, fs, 'yaxis');
    title(['Spectrogram from ', num2str(startTime), 's to ', num2str(endTime), 's']);
    xlabel('Time (s)');
    ylabel('Frequency (Hz)');
    colorbar;
    
    % Adjust x-axis to reflect the actual time of the segment
    xticks(linspace(0, (length(segment)-1)/fs, 5));
    xticklabels(arrayfun(@(x) sprintf('%.2f', segmentTime(1) + x), linspace(0, (length(segment)-1)/fs, 5), 'UniformOutput', false));
end

% Display message
disp('Spectrograms plotted for the specified time intervals.');
