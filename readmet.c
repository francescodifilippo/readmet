#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/**
 * Structure to store a meta tag
 */
typedef struct {
    int type;             // 2=String, 3=Integer
    int nameLength;       // Length of the name
    char *name;           // Tag name
    int valueLength;      // Length of the value (strings only)
    union {
        char *stringValue;  // String value
        int intValue;       // Integer value
    } value;
} MetaTag;

/**
 * Structure to store gap information
 */
typedef struct {
    unsigned int start;   // Gap start position (bytes)
    unsigned int end;     // Gap end position (bytes)
} GapInfo;

/**
 * Structure to store program options
 */
typedef struct {
    int show_special;     // Show special tags
    int show_gap;         // Show gap tags
    int show_standard;    // Show standard tags
    int show_unknown;     // Show unknown tags
    int verbose;          // Verbose output
    int visualize_gaps;   // Visualize gaps
    int json_output;      // Output in JSON format
    
    // Specific field options
    int show_filename;    // Show filename
    int show_filesize;    // Show file size
    int show_date;        // Show last seen date
    int show_progress;    // Show download progress
    int show_hash;        // Show ED2K hash only
    int show_metversion;  // Show .part.met file version only
    int show_tagcount;    // Show number of tags only
    
    char *filename;       // Input filename
} ProgramOptions;

/**
 * Convert a string to uppercase
 * 
 * @param in_str Input string to convert
 * @param out_str Buffer to hold the converted string
 * @return Pointer to out_str
 */
char *strtoupper(char *in_str, char *out_str);

/**
 * Show program usage instructions
 */
void usage(const char *progname) {
    fprintf(stderr, "Usage: %s -f <file> [options]\n", progname);
    fprintf(stderr, "Extract ED2K hash and meta tags from .part.met files\n");
    fprintf(stderr, "\nDisplay options:\n");
    fprintf(stderr, "  -f, --file=FILE      Specify the .part.met file to analyze\n");
    fprintf(stderr, "  -a, --all            Show all tags (default)\n");
    fprintf(stderr, "  -s, --special        Show only special tags\n");
    fprintf(stderr, "  -g, --gap            Show only gap tags\n");
    fprintf(stderr, "  -t, --standard       Show only standard tags\n");
    fprintf(stderr, "  -u, --unknown        Show unknown tags\n");
    fprintf(stderr, "\nSpecific fields (script-friendly, raw output):\n");
    fprintf(stderr, "  -n, --name           Show filename only\n");
    fprintf(stderr, "  -S, --size           Show file size only\n");
    fprintf(stderr, "  -d, --date           Show last seen complete date only\n");
    fprintf(stderr, "  -p, --progress       Show download progress only\n");
    fprintf(stderr, "  -e, --hash           Show ED2K hash only\n");
    fprintf(stderr, "  -m, --metversion     Show .part.met version only (14.0 or 14.1)\n");
    fprintf(stderr, "  -c, --tagcount       Show number of meta tags only\n");
    fprintf(stderr, "\nOutput format:\n");
    fprintf(stderr, "  -j, --json           Output in JSON format\n");
    fprintf(stderr, "\nOther options:\n");
    fprintf(stderr, "  -v, --verbose        Show detailed information\n");
    fprintf(stderr, "  -V, --version        Show program version\n");
    fprintf(stderr, "  -z, --visualize      Visualize file download status\n");
    fprintf(stderr, "  -h, --help           Show this help message\n");
    exit(EXIT_FAILURE);
}

/**
 * Read a byte from the file
 */
unsigned char readByte(int fd) {
    unsigned char byte;
    if (read(fd, &byte, 1) != 1) {
        err(EXIT_FAILURE, "Error reading file");
    }
    return byte;
}

/**
 * Read a word (2 bytes) from the file
 */
unsigned short readWord(int fd) {
    unsigned char bytes[2];
    if (read(fd, bytes, 2) != 2) {
        err(EXIT_FAILURE, "Error reading file");
    }
    return (bytes[0] | (bytes[1] << 8));
}

/**
 * Read a dword (4 bytes) from the file
 */
unsigned int readDWord(int fd) {
    unsigned char bytes[4];
    if (read(fd, bytes, 4) != 4) {
        err(EXIT_FAILURE, "Error reading file");
    }
    return (bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
}

/**
 * Read a string of length len from the file
 */
char *readString(int fd, int len) {
    char *str = (char *)malloc(len + 1);
    if (str == NULL) {
        err(EXIT_FAILURE, "Memory allocation error");
    }
    if (read(fd, str, len) != len) {
        free(str);
        err(EXIT_FAILURE, "Error reading file");
    }
    str[len] = '\0';
    return str;
}

/**
 * Return a description for known special tags
 */
const char *getSpecialTagDescription(int nameValue, int intValue) {
    switch (nameValue) {
        case 1:
            return "Filename";
        case 2:
            return "File size in bytes";
        case 3:
            return "File type";
        case 4:
            return "File format";
        case 5:
            return "Last time file was seen complete on network";
        case 8:
            return "Number of bytes downloaded so far";
        case 18:
            return "Temporary (.part) filename";
        case 19:
            return "Download priority (eDonkey/Overnet <0.49)";
        case 20:
            switch (intValue) {
                case 0: return "Download status: Ready";
                case 1: return "Download status: Empty";
                case 2: return "Download status: Waiting for hash";
                case 3: return "Download status: Hashing";
                case 4: return "Download status: Error";
                case 6: return "Download status: Unknown";
                case 7: return "Download status: Paused";
                case 8: return "Download status: Completing";
                case 9: return "Download status: Completed";
                default: return "Download status: Unknown";
            }
        case 24:
            switch (intValue) {
                case 0: return "Download priority: Low";
                case 1: return "Download priority: Normal";
                case 2: return "Download priority: High";
                case 3: return "Download priority: Very high (eMule) / Highest/Horde (eDonkey/Overnet)";
                case 4: return "Download priority: Very low (eMule)";
                case 5: return "Download priority: Auto (eMule)";
                default: return "Download priority: Unknown";
            }
        case 25:
            switch (intValue) {
                case 0: return "Upload priority: Low";
                case 1: return "Upload priority: Normal";
                case 2: return "Upload priority: High";
                case 3: return "Upload priority: Very high";
                case 4: return "Upload priority: Very low";
                case 5: return "Upload priority: Auto";
                default: return "Upload priority: Unknown";
            }
        default:
            return NULL;
    }
}

/**
 * Return a description for a gap tag
 */
const char *getGapTagDescription(unsigned char firstChar) {
    switch (firstChar) {
        case 9:
            return "Start of gap (undownloaded area)";
        case 10:
            return "End of gap (undownloaded area)";
        default:
            return NULL;
    }
}

/**
 * Return a description for known standard tags
 */
const char *getStandardTagDescription(const char *tagName) {
    if (strcasecmp(tagName, "Artist") == 0) {
        return "Media file artist";
    } else if (strcasecmp(tagName, "Album") == 0) {
        return "Media file album";
    } else if (strcasecmp(tagName, "Title") == 0) {
        return "Media file title";
    } else if (strcasecmp(tagName, "length") == 0) {
        return "Media file duration";
    } else if (strcasecmp(tagName, "bitrate") == 0) {
        return "Media file bitrate";
    } else if (strcasecmp(tagName, "codec") == 0) {
        return "Media file codec";
    }
    return NULL;
}

/**
 * Read and parse a meta tag from the file
 */
MetaTag *readMetaTag(int fd) {
    MetaTag *tag = (MetaTag *)malloc(sizeof(MetaTag));
    if (tag == NULL) {
        err(EXIT_FAILURE, "Memory allocation error");
    }
    
    // Read tag type
    tag->type = readByte(fd);
    
    // Read name length
    tag->nameLength = readWord(fd);
    
    // Read name
    tag->name = readString(fd, tag->nameLength);
    
    // Read value based on type
    if (tag->type == 2) { // String
        tag->valueLength = readWord(fd);
        tag->value.stringValue = readString(fd, tag->valueLength);
    } else if (tag->type == 3) { // Integer
        tag->value.intValue = readDWord(fd);
    } else {
        fprintf(stderr, "Error: Unrecognized tag type: %d\n", tag->type);
        free(tag->name);
        free(tag);
        return NULL;
    }
    
    return tag;
}

/**
 * Free memory used by a meta tag
 */
void freeMetaTag(MetaTag *tag) {
    if (tag != NULL) {
        free(tag->name);
        if (tag->type == 2) { // String
            free(tag->value.stringValue);
        }
        free(tag);
    }
}

/**
 * Determine tag type for filtering
 * Returns: 1=special, 2=gap, 3=standard, 4=unknown
 */
int determineTagType(MetaTag *tag) {
    // Special tag (1-byte name)
    if (tag->nameLength == 1) {
        return 1;
    }
    // Gap tag (name starts with 9 or 10)
    else if (tag->nameLength >= 2 && (tag->name[0] == 9 || tag->name[0] == 10)) {
        return 2;
    }
    // Standard tag or unknown
    else {
        // Convert name to C string
        char tagName[tag->nameLength + 1];
        memcpy(tagName, tag->name, tag->nameLength);
        tagName[tag->nameLength] = '\0';
        
        // Check if it's a known standard tag
        if (getStandardTagDescription(tagName) != NULL) {
            return 3;
        }
        // Unknown tag
        return 4;
    }
}

/**
 * Format a timestamp into a readable date string
 */
char *formatTimestamp(unsigned int timestamp) {
    time_t t = (time_t)timestamp;
    struct tm *tm_info = localtime(&t);
    
    static char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    
    return buffer;
}

/**
 * JSON escape a string
 */
char *jsonEscapeString(const char *str) {
    if (str == NULL) return NULL;
    
    size_t len = strlen(str);
    // Each character could expand up to "\\u00XX" (6 characters)
    // so allocate enough space for the worst case
    size_t escaped_len = len * 6 + 1;
    
    char *escaped = (char *)malloc(escaped_len);
    if (escaped == NULL) {
        return NULL;
    }
    
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (str[i]) {
            case '\\': 
                escaped[j++] = '\\'; 
                escaped[j++] = '\\'; 
                break;
            case '"': 
                escaped[j++] = '\\'; 
                escaped[j++] = '"'; 
                break;
            case '\b': 
                escaped[j++] = '\\'; 
                escaped[j++] = 'b'; 
                break;
            case '\f': 
                escaped[j++] = '\\'; 
                escaped[j++] = 'f'; 
                break;
            case '\n': 
                escaped[j++] = '\\'; 
                escaped[j++] = 'n'; 
                break;
            case '\r': 
                escaped[j++] = '\\'; 
                escaped[j++] = 'r'; 
                break;
            case '\t': 
                escaped[j++] = '\\'; 
                escaped[j++] = 't'; 
                break;
            default:
                if ((unsigned char)str[i] < 32) {
                    // Non-printable control characters
                    sprintf(escaped + j, "\\u%04x", (unsigned char)str[i]);
                    j += 6;
                } else {
                    escaped[j++] = str[i];
                }
                break;
        }
    }
    escaped[j] = '\0';
    
    return escaped;
}

/**
 * Print meta tag information with optional verbosity
 */
void printMetaTag(MetaTag *tag, int verbose, int json_output) {
    int tagType = determineTagType(tag);
    
    if (json_output) {
        printf("{\"type\":");
        
        // Tag type
        switch (tagType) {
            case 1: printf("\"special\""); break;
            case 2: printf("\"gap\""); break;
            case 3: printf("\"standard\""); break;
            case 4: printf("\"unknown\""); break;
        }
        
        // Tag ID for special tags
        if (tagType == 1) {
            printf(",\"id\":%d", (unsigned char)tag->name[0]);
        }
        
        // Tag name for non-special tags
        if (tagType != 1) {
            if (tagType == 2) {
                // Gap tag
                unsigned char firstChar = tag->name[0];
                printf(",\"gap_type\":");
                if (firstChar == 9) {
                    printf("\"start\"");
                } else if (firstChar == 10) {
                    printf("\"end\"");
                } else {
                    printf("\"unknown\"");
                }
                
                // Extract reference number
                if (tag->nameLength > 1) {
                    char refNum[tag->nameLength];
                    memcpy(refNum, tag->name + 1, tag->nameLength - 1);
                    refNum[tag->nameLength - 1] = '\0';
                    printf(",\"reference\":\"%s\"", refNum);
                }
            } else {
                // Standard or unknown tag
                char tagName[tag->nameLength + 1];
                memcpy(tagName, tag->name, tag->nameLength);
                tagName[tag->nameLength] = '\0';
                
                char *escapedName = jsonEscapeString(tagName);
                printf(",\"name\":\"%s\"", escapedName ? escapedName : "");
                free(escapedName);
            }
        }
        
        // Description for known tags
        if (tagType == 1) {
            const char *desc = getSpecialTagDescription((unsigned char)tag->name[0], 
                                                       tag->type == 3 ? tag->value.intValue : 0);
            if (desc) {
                char *escapedDesc = jsonEscapeString(desc);
                printf(",\"description\":\"%s\"", escapedDesc ? escapedDesc : "");
                free(escapedDesc);
            }
        }
        
        // Value
        if (tag->type == 3) { // Integer
            printf(",\"value\":%d", tag->value.intValue);
            
            // Add additional info for certain special tags
            if (tagType == 1) {
                unsigned char nameValue = tag->name[0];
                if (nameValue == 2 || nameValue == 8) { // File size or downloaded bytes
                    printf(",\"value_mb\":%.2f", tag->value.intValue / 1048576.0);
                } else if (nameValue == 5) { // Last seen date
                    printf(",\"value_date\":\"%s\"", formatTimestamp(tag->value.intValue));
                }
            }
        } else { // String
            char *escapedValue = jsonEscapeString(tag->value.stringValue);
            printf(",\"value\":\"%s\"", escapedValue ? escapedValue : "");
            free(escapedValue);
        }
        
        printf("}");
    } else {
        // Special tag (1-byte name)
        if (tagType == 1) {
            unsigned char nameValue = tag->name[0];
            printf("Tag: (Special, %d) ", nameValue);
            
            const char *desc = NULL;
            if (tag->type == 3) { // Integer
                desc = getSpecialTagDescription(nameValue, tag->value.intValue);
                if (desc) {
                    printf("%s = %d", desc, tag->value.intValue);
                    
                    // Extra details for certain special tags in verbose mode
                    if (verbose) {
                        if (nameValue == 2) { // File size
                            printf(" (%.2f MB)", tag->value.intValue / 1048576.0);
                        } else if (nameValue == 8) { // Downloaded bytes
                            printf(" (%.2f MB)", tag->value.intValue / 1048576.0);
                        } else if (nameValue == 5) { // Last seen date
                            printf(" (%s)", formatTimestamp(tag->value.intValue));
                        } else if (nameValue == 20) { // Status
                            switch(tag->value.intValue) {
                                case 0: printf(" - File is ready for download"); break;
                                case 7: printf(" - Download is manually paused"); break;
                                case 9: printf(" - Download is fully completed"); break;
                            }
                        }
                    }
                } else {
                    printf("Name: %d, Value: %d", nameValue, tag->value.intValue);
                }
            } else { // String
                desc = getSpecialTagDescription(nameValue, 0);
                if (desc) {
                    printf("%s = \"%s\"", desc, tag->value.stringValue);
                } else {
                    printf("Name: %d, Value: \"%s\"", nameValue, tag->value.stringValue);
                }
            }
        }
        // Gap tag
        else if (tagType == 2) {
            const char *desc = getGapTagDescription(tag->name[0]);
            if (desc) {
                // Extract reference number
                char refNum[tag->nameLength];
                memcpy(refNum, tag->name + 1, tag->nameLength - 1);
                refNum[tag->nameLength - 1] = '\0';
                
                printf("Tag: (Gap) %s, Reference: %s", desc, refNum);
                
                if (tag->type == 3) { // Integer
                    printf(", Value: %d", tag->value.intValue);
                    if (verbose) {
                        printf(" (%.2f MB)", tag->value.intValue / 1048576.0);
                    }
                } else { // String
                    printf(", Value: \"%s\"", tag->value.stringValue);
                }
            } else {
                printf("Tag: Unrecognized gap tag");
            }
        }
        // Standard or unknown tag
        else {
            // Convert name to C string
            char tagName[tag->nameLength + 1];
            memcpy(tagName, tag->name, tag->nameLength);
            tagName[tag->nameLength] = '\0';
            
            const char *desc = getStandardTagDescription(tagName);
            if (desc) {
                printf("Tag: (Standard) %s = ", tagName);
                if (tag->type == 3) { // Integer
                    printf("%d", tag->value.intValue);
                } else { // String
                    printf("\"%s\"", tag->value.stringValue);
                }
                if (verbose) {
                    printf(" - %s", desc);
                }
            } else {
                printf("Tag: (Unknown) Name: \"%s\", ", tagName);
                if (tag->type == 3) { // Integer
                    printf("Value: %d", tag->value.intValue);
                } else { // String
                    printf("Value: \"%s\"", tag->value.stringValue);
                }
            }
        }
        
        printf("\n");
    }
}

/**
 * Display specific field information
 */
void displaySpecificField(MetaTag **tags, int numTags, int fieldType, int verbose, int json_output) {
    // Look for the specific field
    for (int i = 0; i < numTags; i++) {
        if (tags[i]->nameLength == 1 && tags[i]->name[0] == fieldType) {
            switch (fieldType) {
                case 1: // Filename
                    if (tags[i]->type == 2) {
                        if (json_output) {
                            char *escapedValue = jsonEscapeString(tags[i]->value.stringValue);
                            printf("{\"filename\":\"%s\"}", escapedValue ? escapedValue : "");
                            free(escapedValue);
                        } else {
                            printf("%s", tags[i]->value.stringValue);
                        }
                    }
                    break;
                    
                case 2: // File size
                    if (tags[i]->type == 3) {
                        if (json_output) {
                            printf("{\"filesize\":%u", tags[i]->value.intValue);
                            if (verbose) {
                                printf(",\"filesize_mb\":%.2f", tags[i]->value.intValue / 1048576.0);
                            }
                            printf("}");
                        } else {
                            printf("%u", tags[i]->value.intValue);
                        }
                    }
                    break;
                    
                case 5: // Last seen date
                    if (tags[i]->type == 3) {
                        if (json_output) {
                            printf("{\"last_seen\":%u", tags[i]->value.intValue);
                            if (verbose) {
                                printf(",\"last_seen_date\":\"%s\"", formatTimestamp(tags[i]->value.intValue));
                            }
                            printf("}");
                        } else {
                            if (verbose) {
                                printf("%s", formatTimestamp(tags[i]->value.intValue));
                            } else {
                                printf("%u", tags[i]->value.intValue);
                            }
                        }
                    }
                    break;
            }
            // Each specific field should only be found once
            return;
        }
    }
    
    // Field not found
    if (json_output) {
        switch (fieldType) {
            case 1:
                printf("{\"filename\":null}");
                break;
            case 2:
                printf("{\"filesize\":null}");
                break;
            case 5:
                printf("{\"last_seen\":null}");
                break;
        }
    } else {
        // For script usage, output nothing if field not found
    }
}

/**
 * Display download progress information
 */
void displayProgress(unsigned int fileSize, unsigned int downloadedBytes, int json_output) {
    double percentage = 0.0;
    if (fileSize > 0) {
        percentage = (downloadedBytes * 100.0) / fileSize;
    }
    
    if (json_output) {
        printf("{\"total_bytes\":%u,\"downloaded_bytes\":%u,\"total_mb\":%.2f,\"downloaded_mb\":%.2f,\"percentage\":%.1f}",
               fileSize, downloadedBytes,
               fileSize / 1048576.0, downloadedBytes / 1048576.0,
               percentage);
    } else {
        // For script usage, just output the percentage
        printf("%.1f", percentage);
    }
}

/**
 * Collect gap information from all gap tags into an array
 */
GapInfo* collectGaps(MetaTag **tags, int numTags, int *numGaps) {
    GapInfo *gaps = NULL;
    int gapCount = 0;
    
    // First, count the number of gap pairs
    for (int i = 0; i < numTags; i++) {
        if (tags[i]->nameLength >= 2 && tags[i]->name[0] == 9) { // Start of gap
            gapCount++;
        }
    }
    
    // Allocate memory for gaps
    gaps = (GapInfo *)malloc(gapCount * sizeof(GapInfo));
    if (gaps == NULL && gapCount > 0) {
        err(EXIT_FAILURE, "Memory allocation error");
    }
    
    // Match start and end gaps
    int gapIndex = 0;
    for (int i = 0; i < numTags; i++) {
        // Find start gap tag
        if (tags[i]->nameLength >= 2 && tags[i]->name[0] == 9 && tags[i]->type == 3) {
            char refNum[tags[i]->nameLength];
            memcpy(refNum, tags[i]->name + 1, tags[i]->nameLength - 1);
            refNum[tags[i]->nameLength - 1] = '\0';
            
            unsigned int startPos = tags[i]->value.intValue;
            unsigned int endPos = 0;
            
            // Find matching end gap
            for (int j = 0; j < numTags; j++) {
                if (tags[j]->nameLength >= 2 && tags[j]->name[0] == 10 && tags[j]->type == 3) {
                    char endRefNum[tags[j]->nameLength];
                    memcpy(endRefNum, tags[j]->name + 1, tags[j]->nameLength - 1);
                    endRefNum[tags[j]->nameLength - 1] = '\0';
                    
                    if (strcmp(refNum, endRefNum) == 0) {
                        endPos = tags[j]->value.intValue;
                        break;
                    }
                }
            }
            
            if (endPos > 0 && gapIndex < gapCount) {
                gaps[gapIndex].start = startPos;
                gaps[gapIndex].end = endPos;
                gapIndex++;
            }
        }
    }
    
    *numGaps = gapIndex;
    return gaps;
}

/**
 * Visualize file download status with gaps
 */
void visualizeFileStatus(GapInfo *gaps, int numGaps, unsigned int fileSize, unsigned int downloadedBytes, int json_output) {
    const int barWidth = 70; // Width of visualization bar
    
    if (json_output) {
        printf("{\"visualization\":{");
        printf("\"total_size\":%u,\"total_size_mb\":%.2f,", fileSize, fileSize / 1048576.0);
        printf("\"downloaded\":%u,\"downloaded_mb\":%.2f,", downloadedBytes, downloadedBytes / 1048576.0);
        double perc = 0.0;
        if (fileSize > 0) {
            perc = (downloadedBytes * 100.0) / fileSize;
        }
        printf("\"percentage\":%.1f,", perc);
        
        // Gap statistics
        printf("\"gaps\":{\"count\":%d,", numGaps);
        
        if (numGaps > 0) {
            unsigned int totalGapSize = 0;
            for (int i = 0; i < numGaps; i++) {
                totalGapSize += (gaps[i].end - gaps[i].start);
            }
            
            double gapPerc = 0.0;
            if (fileSize > 0) {
                gapPerc = (totalGapSize * 100.0) / fileSize;
            }
            printf("\"total_size\":%u,\"total_size_mb\":%.2f,\"percentage\":%.1f,",
                   totalGapSize, totalGapSize / 1048576.0, gapPerc);
            
            // Add gap details
            printf("\"details\":[");
            for (int i = 0; i < numGaps; i++) {
                printf("{\"start\":%u,\"end\":%u,\"size\":%u,\"size_mb\":%.2f}",
                       gaps[i].start, gaps[i].end,
                       gaps[i].end - gaps[i].start,
                       (gaps[i].end - gaps[i].start) / 1048576.0);
                
                if (i < numGaps - 1) {
                    printf(",");
                }
            }
            printf("]");
        } else {
            printf("\"total_size\":0,\"total_size_mb\":0.0,\"percentage\":0.0,\"details\":[]");
        }
        
        printf("}");  // Close gaps object
        
        // Visual representation as array
        printf(",\"bar\":[");
        for (int i = 0; i < barWidth; i++) {
            // Calculate file position this bar position represents
            unsigned int posStart = (unsigned int)((i / (double)barWidth) * fileSize);
            unsigned int posEnd = (unsigned int)(((i + 1) / (double)barWidth) * fileSize);
            
            // Check if this position is in a gap
            int inGap = 0;
            for (int j = 0; j < numGaps; j++) {
                // If there's any overlap between this bar position and a gap
                if (!(posEnd <= gaps[j].start || posStart >= gaps[j].end)) {
                    inGap = 1;
                    break;
                }
            }
            
            printf("%d", inGap ? 0 : 1);
            if (i < barWidth - 1) {
                printf(",");
            }
        }
        printf("]");
        
        printf("}}");  // Close visualization and outer objects
    } else {
        printf("\n=== FILE DOWNLOAD VISUALIZATION ===\n");
        
        // Show basic info
        printf("Total size: %u bytes (%.2f MB)\n", fileSize, fileSize / 1048576.0);
        double perc = 0.0;
        if (fileSize > 0) {
            perc = (downloadedBytes * 100.0) / fileSize;
        }
        printf("Downloaded: %u bytes (%.2f MB, %.1f%%)\n",
               downloadedBytes,
               downloadedBytes / 1048576.0,
               perc);
        
        // Draw progress bar
        printf("[");
        
        // For each position in the progress bar
        for (int i = 0; i < barWidth; i++) {
            // Calculate file position this bar position represents
            unsigned int posStart = (unsigned int)((i / (double)barWidth) * fileSize);
            unsigned int posEnd = (unsigned int)(((i + 1) / (double)barWidth) * fileSize);
            
            // Check if this position is in a gap
            int inGap = 0;
            for (int j = 0; j < numGaps; j++) {
                // If there's any overlap between this bar position and a gap
                if (!(posEnd <= gaps[j].start || posStart >= gaps[j].end)) {
                    inGap = 1;
                    break;
                }
            }
            
            // Print character based on gap status
            if (inGap) {
                printf(" "); // Gap/missing part
            } else {
                printf("#"); // Downloaded part
            }
        }
        
        printf("]\n\n");
        
        // Show gap statistics
        if (numGaps > 0) {
            printf("Gaps: %d\n", numGaps);
            
            // Calculate total gap size
            unsigned int totalGapSize = 0;
            for (int i = 0; i < numGaps; i++) {
                totalGapSize += (gaps[i].end - gaps[i].start);
            }
            
            double gapPerc = 0.0;
            if (fileSize > 0) {
                gapPerc = (totalGapSize * 100.0) / fileSize;
            }
            printf("Total gap size: %.2f MB (%.1f%% of file)\n\n",
                   totalGapSize / 1048576.0,
                   gapPerc);
        }
    }
}

int main(int argc, char **argv) {
    extern char *optarg;
    extern int optind;
    
    int ch, fd = -1;
    char *buffer;
    char *ed2khash;
    char *uped2khash;
    int starthash = 5;
    int show_version = 0;
    int metVersion = 0;
    unsigned int fileSize = 0;
    unsigned int downloadedBytes = 0;
    
    // Default options
    ProgramOptions options = {
        .show_special = 0,
        .show_gap = 0,
        .show_standard = 0,
        .show_unknown = 0,
        .verbose = 0,
        .visualize_gaps = 0,
        .json_output = 0,
        .show_filename = 0,
        .show_filesize = 0,
        .show_date = 0,
        .show_progress = 0,
        .show_hash = 0,
        .show_metversion = 0,
        .show_tagcount = 0,
        .filename = NULL
    };
    
    static struct option longopts[] = {
        { "file",      required_argument, NULL, 'f' },
        { "all",       no_argument,       NULL, 'a' },
        { "special",   no_argument,       NULL, 's' },
        { "gap",       no_argument,       NULL, 'g' },
        { "standard",  no_argument,       NULL, 't' },
        { "unknown",   no_argument,       NULL, 'u' },
        { "name",      no_argument,       NULL, 'n' },
        { "size",      no_argument,       NULL, 'S' },
        { "date",      no_argument,       NULL, 'd' },
        { "progress",  no_argument,       NULL, 'p' },
        { "hash",      no_argument,       NULL, 'e' },
        { "metversion",no_argument,       NULL, 'm' },
        { "tagcount",  no_argument,       NULL, 'c' },
        { "json",      no_argument,       NULL, 'j' },
        { "verbose",   no_argument,       NULL, 'v' },
        { "version",   no_argument,       NULL, 'V' },
        { "visualize", no_argument,       NULL, 'z' },
        { "help",      no_argument,       NULL, 'h' },
        { NULL,        0,                 NULL,  0  }
    };
    
    // If no arguments, show usage
    if (argc == 1) {
        usage(argv[0]);
    }
    
    // Parse command line arguments
    while ((ch = getopt_long(argc, argv, "f:asgtunSdpemcjvVzh", longopts, NULL)) != -1) {
        switch (ch) {
            case 'f':
                options.filename = optarg;
                if ((fd = open(optarg, O_RDONLY)) == -1)
                    err(EXIT_FAILURE, "Unable to open file %s", optarg);
                break;
            case 'a':
                options.show_special = 1;
                options.show_gap = 1;
                options.show_standard = 1;
                options.show_unknown = 1;
                break;
            case 's':
                options.show_special = 1;
                break;
            case 'g':
                options.show_gap = 1;
                break;
            case 't':
                options.show_standard = 1;
                break;
            case 'u':
                options.show_unknown = 1;
                break;
            case 'n':
                options.show_filename = 1;
                break;
            case 'S':
                options.show_filesize = 1;
                break;
            case 'd':
                options.show_date = 1;
                break;
            case 'p':
                options.show_progress = 1;
                break;
            case 'e':
                options.show_hash = 1;
                break;
            case 'm':
                options.show_metversion = 1;
                break;
            case 'c':
                options.show_tagcount = 1;
                break;
            case 'j':
                options.json_output = 1;
                break;
            case 'v':
                options.verbose = 1;
                break;
            case 'V':
                show_version = 1;
                break;
            case 'z':
                options.visualize_gaps = 1;
                break;
            case 'h':
                usage(argv[0]);
                break;
            default:
                usage(argv[0]);
        }
    }
    
    // If no tag filters or specific fields specified, show all by default
    if (!options.show_special && !options.show_gap && !options.show_standard && 
        !options.show_unknown && !options.show_filename && !options.show_filesize && 
        !options.show_date && !options.show_progress && !options.visualize_gaps &&
        !options.show_hash && !options.show_metversion && !options.show_tagcount) {
        options.show_special = 1;
        options.show_gap = 1;
        options.show_standard = 1;
        options.show_unknown = 1;
    }
    
    if (show_version) {
        if (options.json_output) {
            printf("{\"version\":\"readmet v1.0\",\"based_on\":\"ed2k .part.met file format document by Ivan Montes (Dr.Slump)\"}");
        } else {
            printf("readmet v1.0\n");
            printf("Based on 'ed2k .part.met file format' document by Ivan Montes (Dr.Slump)\n");
        }
        
        // If no file was specified, exit
        if (fd == -1) {
            exit(EXIT_SUCCESS);
        }
    }
    
    if (fd == -1) {
        fprintf(stderr, "Error: You must specify a .part.met file\n");
        usage(argv[0]);
    }
    
    // JSON output start
    if (options.json_output && 
        !(options.show_hash || options.show_metversion || options.show_tagcount || 
          options.show_filename || options.show_filesize || options.show_date || 
          options.show_progress)) {
        printf("{");
    }
    
    // Read first byte to determine file version
    buffer = (char *)calloc(1, 2);
    if (buffer == NULL) {
        err(EXIT_FAILURE, "Memory allocation error");
    }
    
    if (read(fd, buffer, 1) != 1) {
        free(buffer);
        close(fd);
        err(EXIT_FAILURE, "Error reading file");
    }
    
    buffer[1] = '\0';
    
    // Determine hash position based on file version
    char *versionStr = NULL;
    switch ((int)buffer[0] & 0xff) {
        case 224:
            starthash = 5;
            metVersion = 0; // Version 14.0
            versionStr = "14.0";
            if (options.show_metversion) {
                // Output only the version number when specifically requested
                if (options.json_output) {
                    printf("{\"format_version\":\"%s\"}", versionStr);
                } else {
                    printf("%s", versionStr);
                }
                free(buffer);
                close(fd);
                return EXIT_SUCCESS;
            } else if (options.json_output && 
                      !(options.show_hash || options.show_tagcount || 
                        options.show_filename || options.show_filesize || 
                        options.show_date || options.show_progress)) {
                printf("\"format_version\":\"%s\",", versionStr);
            } else if (!options.json_output && 
                      !(options.show_hash || options.show_tagcount || 
                        options.show_filename || options.show_filesize || 
                        options.show_date || options.show_progress)) {
                printf(".part.met file version: %s\n", versionStr);
            }
            break;
        case 225:
            starthash = 6;
            metVersion = 1; // Version 14.1
            versionStr = "14.1";
            if (options.show_metversion) {
                // Output only the version number when specifically requested
                if (options.json_output) {
                    printf("{\"format_version\":\"%s\"}", versionStr);
                } else {
                    printf("%s", versionStr);
                }
                free(buffer);
                close(fd);
                return EXIT_SUCCESS;
            } else if (options.json_output && 
                      !(options.show_hash || options.show_tagcount || 
                        options.show_filename || options.show_filesize || 
                        options.show_date || options.show_progress)) {
                printf("\"format_version\":\"%s\",", versionStr);
            } else if (!options.json_output && 
                      !(options.show_hash || options.show_tagcount || 
                        options.show_filename || options.show_filesize || 
                        options.show_date || options.show_progress)) {
                printf(".part.met file version: %s\n", versionStr);
            }
            break;
        default:
            free(buffer);
            close(fd);
            errx(EXIT_FAILURE, "Unrecognized or invalid file format");
    }
    
    // Position file pointer at hash location
    if (lseek(fd, starthash, SEEK_SET) == -1) {
        free(buffer);
        close(fd);
        err(EXIT_FAILURE, "Error positioning within file");
    }
    
    free(buffer);
    
    // Allocate memory for hash and its string representation
    buffer = (char *)calloc(1, 17);
    ed2khash = (char *)malloc(sizeof(char) * 33);
    uped2khash = (char *)malloc(sizeof(char) * 33);
    
    if (buffer == NULL || ed2khash == NULL || uped2khash == NULL) {
        free(buffer);
        free(ed2khash);
        free(uped2khash);
        close(fd);
        err(EXIT_FAILURE, "Memory allocation error");
    }
    
    // Read 16 bytes of the hash
    if (read(fd, buffer, 16) != 16) {
        free(buffer);
        free(ed2khash);
        free(uped2khash);
        close(fd);
        err(EXIT_FAILURE, "Error reading hash");
    }
    
    buffer[16] = '\0';
    
    // Format hash as hexadecimal string
    sprintf(ed2khash, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
            buffer[0] & 0xff, buffer[1] & 0xff, buffer[2] & 0xff, buffer[3] & 0xff,
            buffer[4] & 0xff, buffer[5] & 0xff, buffer[6] & 0xff, buffer[7] & 0xff,
            buffer[8] & 0xff, buffer[9] & 0xff, buffer[10] & 0xff, buffer[11] & 0xff,
            buffer[12] & 0xff, buffer[13] & 0xff, buffer[14] & 0xff, buffer[15] & 0xff);
    
    // Convert hash to uppercase
    strtoupper(ed2khash, uped2khash);
    
    // Handle -e/--hash option specially
    if (options.show_hash) {
        if (options.json_output) {
            printf("{\"ed2k_hash\":\"%s\"}", uped2khash);
        } else {
            printf("%s", uped2khash);
        }
        
        // Free hash memory
        free(buffer);
        free(ed2khash);
        free(uped2khash);
        close(fd);
        return EXIT_SUCCESS;
    }
    
    // Print hash in normal mode
    if (options.json_output && 
        !(options.show_tagcount || options.show_filename || 
          options.show_filesize || options.show_date || options.show_progress)) {
        printf("\"ed2k_hash\":\"%s\",", uped2khash);
    } else if (!options.json_output && 
              !(options.show_tagcount || options.show_filename || 
                options.show_filesize || options.show_date || options.show_progress)) {
        printf("ED2K Hash: %s\n", uped2khash);
    }
    
    // Free hash memory
    free(buffer);
    free(ed2khash);
    free(uped2khash);
    
    // Position for reading meta tags
    int numTagsPosition;
    int numBlocks = 0;
    
    if (metVersion == 0) { // Version 14.0
        // In version 14.0 we need to read the number of blocks first
        lseek(fd, 21, SEEK_SET); // Position of 'Blocks' field
        numBlocks = readWord(fd);
        
        // Calculate position of NumTags field
        numTagsPosition = 23 + (16 * numBlocks);
        lseek(fd, numTagsPosition, SEEK_SET);
    } else { // Version 14.1
        // In version 14.1, NumTags is right after the ED2K hash
        numTagsPosition = 22;
        lseek(fd, numTagsPosition, SEEK_SET);
    }
    
    // Read number of meta tags
    unsigned int numTags = readDWord(fd);
    
    // Handle -c/--tagcount option specially
    if (options.show_tagcount) {
        if (options.json_output) {
            printf("{\"num_tags\":%u}", numTags);
        } else {
            printf("%u", numTags);
        }
        close(fd);
        return EXIT_SUCCESS;
    }
    
    if (options.json_output && 
        !(options.show_filename || options.show_filesize || 
          options.show_date || options.show_progress)) {
        printf("\"num_tags\":%u,", numTags);
    } else if (!options.json_output && 
              !(options.show_filename || options.show_filesize || 
                options.show_date || options.show_progress)) {
        printf("Number of meta tags: %u\n", numTags);
    }
    
    // Read all meta tags into memory
    MetaTag **tags = (MetaTag **)malloc(numTags * sizeof(MetaTag *));
    if (tags == NULL) {
        close(fd);
        err(EXIT_FAILURE, "Memory allocation error");
    }
    
    for (unsigned int i = 0; i < numTags; i++) {
        tags[i] = readMetaTag(fd);
        if (tags[i] == NULL) {
            // Free previously allocated tags
            for (unsigned int j = 0; j < i; j++) {
                freeMetaTag(tags[j]);
            }
            free(tags);
            close(fd);
            err(EXIT_FAILURE, "Error reading meta tags");
        }
        
        // Keep track of file size and downloaded bytes for visualization
        if (tags[i]->nameLength == 1) {
            if (tags[i]->name[0] == 2 && tags[i]->type == 3) { // File size
                fileSize = tags[i]->value.intValue;
            } else if (tags[i]->name[0] == 8 && tags[i]->type == 3) { // Downloaded bytes
                downloadedBytes = tags[i]->value.intValue;
            }
        }
    }
    
    // Output structure for specific fields
    if (options.show_filename || options.show_filesize || 
        options.show_date || options.show_progress) {
        
        // In JSON mode, we need a separate "fields" object
        if (options.json_output) {
            printf("{\"fields\":{");
        }
        
        int fieldsOutput = 0;
        
        if (options.show_filename) {
            displaySpecificField(tags, numTags, 1, options.verbose, options.json_output);
            fieldsOutput++;
            // We want to exit immediately for script usage mode
            if (!options.json_output) {
                // Free all tags
                for (unsigned int i = 0; i < numTags; i++) {
                    freeMetaTag(tags[i]);
                }
                free(tags);
                close(fd);
                return EXIT_SUCCESS;
            }
        }
        
        if (options.show_filesize) {
            // Add comma if needed in JSON mode
            if (options.json_output && fieldsOutput > 0) {
                printf(",");
            }
            displaySpecificField(tags, numTags, 2, options.verbose, options.json_output);
            fieldsOutput++;
            // We want to exit immediately for script usage mode
            if (!options.json_output) {
                // Free all tags
                for (unsigned int i = 0; i < numTags; i++) {
                    freeMetaTag(tags[i]);
                }
                free(tags);
                close(fd);
                return EXIT_SUCCESS;
            }
        }
        
        if (options.show_date) {
            // Add comma if needed in JSON mode
            if (options.json_output && fieldsOutput > 0) {
                printf(",");
            }
            displaySpecificField(tags, numTags, 5, options.verbose, options.json_output);
            fieldsOutput++;
            // We want to exit immediately for script usage mode
            if (!options.json_output) {
                // Free all tags
                for (unsigned int i = 0; i < numTags; i++) {
                    freeMetaTag(tags[i]);
                }
                free(tags);
                close(fd);
                return EXIT_SUCCESS;
            }
        }
        
        if (options.show_progress) {
            // Add comma if needed in JSON mode
            if (options.json_output && fieldsOutput > 0) {
                printf(",");
            }
            
            if (options.json_output) {
                printf("\"progress\":");
            }
            
            displayProgress(fileSize, downloadedBytes, options.json_output);
            fieldsOutput++;
            // We want to exit immediately for script usage mode
            if (!options.json_output) {
                // Free all tags
                for (unsigned int i = 0; i < numTags; i++) {
                    freeMetaTag(tags[i]);
                }
                free(tags);
                close(fd);
                return EXIT_SUCCESS;
            }
        }
        
        // Close the "fields" object in JSON mode
        if (options.json_output) {
            printf("}}");
            // Exit - we're done with specific fields in JSON mode
            // Free all tags
            for (unsigned int i = 0; i < numTags; i++) {
                freeMetaTag(tags[i]);
            }
            free(tags);
            close(fd);
            return EXIT_SUCCESS;
        }
    }
    
    // Display tags based on filter options
    if (options.show_special || options.show_gap || 
        options.show_standard || options.show_unknown) {
        
        // Start the tags array in JSON mode
        if (options.json_output) {
            printf("\"tags\":[");
        } else {
            printf("\n=== META TAGS ===\n");
        }
        
        int tagsOutput = 0;
        
        for (unsigned int i = 0; i < numTags; i++) {
            int tagType = determineTagType(tags[i]);
            
            // Apply filters
            if ((tagType == 1 && options.show_special) ||
                (tagType == 2 && options.show_gap) ||
                (tagType == 3 && options.show_standard) ||
                (tagType == 4 && options.show_unknown)) {
                
                // Add comma if needed in JSON mode
                if (options.json_output && tagsOutput > 0) {
                    printf(",");
                }
                
                printMetaTag(tags[i], options.verbose, options.json_output);
                tagsOutput++;
            }
        }
        
        // Close the tags array in JSON mode
        if (options.json_output) {
            printf("]");
        }
    }
    
    // Visualize file status if requested
    if (options.visualize_gaps) {
        GapInfo *gaps;
        int numGaps;
        
        gaps = collectGaps(tags, numTags, &numGaps);
        
        // Add comma if needed in JSON mode
        if (options.json_output && (options.show_special || options.show_gap || 
            options.show_standard || options.show_unknown)) {
            printf(",");
        }
        
        visualizeFileStatus(gaps, numGaps, fileSize, downloadedBytes, options.json_output);
        
        free(gaps);
    }
    
    // Close the JSON output
    if (options.json_output && 
        !(options.show_hash || options.show_metversion || options.show_tagcount || 
          options.show_filename || options.show_filesize || options.show_date || 
          options.show_progress)) {
        printf("}\n");
    }
    
    // Free all tags
    for (unsigned int i = 0; i < numTags; i++) {
        freeMetaTag(tags[i]);
    }
    free(tags);
    
    // Close file and exit
    close(fd);
    return EXIT_SUCCESS;
}

/**
 * Convert a string to uppercase
 */
char *strtoupper(char *in_str, char *out_str) {
    int index;
    
    for (index = 0; in_str[index]; index++)
        out_str[index] = toupper(in_str[index]);
    
    out_str[index] = 0;
    
    return out_str;
}
