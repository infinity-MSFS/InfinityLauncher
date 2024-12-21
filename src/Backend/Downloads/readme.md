# Downloader

## About

Wrappers for interacting with zoe asynchronously

## Usage Example

```c++
  auto &downloader = Infinity::Downloads::GetInstance(); // Get a pointer to the downloader singeton
  
        if (ImGui::Button("Download")) {
            // Start download (std::string url, std::string download_path), returns the download ID used to control download
            downloadID = downloader.StartDownload("http://link.testfile.org/150MB", "/home/katelyn/test/test_file.zip");
        }
        
        if (downloadID != -1) {
            auto *downloadDataPtr = downloader.GetDownloadData(downloadID);
           if (downloadDataPtr) {
                ImGui::Text("Downloading...");
                ImGui::Text("Download State: %d", downloadDataPtr->zoe->state()); // See zoe::Result
    
                if (ImGui::Button("Pause")) {
                    downloader.PauseDownload(downloadID); // Pass the generated ID to pause the download
                }
                if (ImGui::Button("Resume")) {
                    downloader.ResumeDownload(downloadID); // Pass the generated ID to resume a paused download
                }
                if (ImGui::Button("Cancel")) {
                    downloader.StopDownload(downloadID); // Pass the generated ID to cancel a download
                }
                ImGui::Text("Download Progress: %.2f", downloadDataPtr->progress); // 0.0f - 1.0f (seems to stop at 0.(9))
            }
            // see Downloads::DownloadData for all the data that can be consumed from the pointer
        }
```