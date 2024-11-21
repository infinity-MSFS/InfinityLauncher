use std::{env};
use std::fs::File;
use std::path::Path;
use std::str::Chars;
use std::io::{self, Read,Write};

#[derive(Debug, Clone)]
enum PathCommand {
    MoveTo { x: f64, y: f64 },
    LineTo { x: f64, y: f64 },
    CubicBezier {
        x1: f64,
        y1: f64,
        x2: f64,
        y2: f64,
        x: f64,
        y: f64,
    },
    SmoothCubicBezier {
        x2: f64,
        y2: f64,
        x: f64,
        y: f64,
    },
    ClosePath,
}

struct PathFlattener {
    resolution: f64,
    current_point: (f64, f64),
    last_control_point: Option<(f64, f64)>,
}

impl PathFlattener {
    fn new(resolution: f64) -> Self {
        PathFlattener {
            resolution,
            current_point: (0.0, 0.0),
            last_control_point: None,
        }
    }

    fn cubic_bezier(&self, t: f64, p0: (f64, f64), p1: (f64, f64), p2: (f64, f64), p3: (f64, f64)) -> (f64, f64) {
        let u = 1.0 - t;
        let tt = t * t;
        let uu = u * u;
        let uuu = uu * u;
        let ttt = tt * t;

        (
            uuu * p0.0 + 3.0 * uu * t * p1.0 + 3.0 * u * tt * p2.0 + ttt * p3.0,
            uuu * p0.1 + 3.0 * uu * t * p1.1 + 3.0 * u * tt * p2.1 + ttt * p3.1
        )
    }

    fn flatten_command(&mut self, cmd: &PathCommand) -> Vec<(f64, f64)> {
        let mut segments = Vec::new();

        match cmd {
            PathCommand::MoveTo { x, y } => {
                self.current_point = (*x, *y);
                segments.push(self.current_point);
            }
            PathCommand::LineTo { x, y } => {
                self.current_point = (*x, *y);
                segments.push(self.current_point);
            }
            PathCommand::CubicBezier { x1, y1, x2, y2, x, y } => {
                let start = self.current_point;
                let control1 = (*x1, *y1);
                let control2 = (*x2, *y2);
                let end = (*x, *y);

                let mut t = 0.0;
                while t <= 1.0 {
                    let point = self.cubic_bezier(t, start, control1, control2, end);
                    segments.push(point);
                    t += self.resolution;
                }

                segments.push(end);

                self.last_control_point = Some((control2.0, control2.1));
                self.current_point = end;
            }
            PathCommand::SmoothCubicBezier { x2, y2, x, y } => {
                let start = self.current_point;
                let control1 = self.last_control_point.map_or(start, |last_cp| {
                    (
                        start.0 + (start.0 - last_cp.0),
                        start.1 + (start.1 - last_cp.1)
                    )
                });
                let control2 = (*x2, *y2);
                let end = (*x, *y);

                let mut t = 0.0;
                while t <= 1.0 {
                    let point = self.cubic_bezier(t, start, control1, control2, end);
                    segments.push(point);
                    t += self.resolution;
                }

                segments.push(end);

                self.last_control_point = Some((control2.0, control2.1));
                self.current_point = end;
            }
            PathCommand::ClosePath => {}
        }

        segments
    }
}

fn parse_path(path_data: &str) -> Vec<PathCommand> {
    let mut commands = Vec::new();
    let mut chars = path_data.chars().peekable();

    while let Some(&cmd) = chars.peek() {
        match cmd {
            'M' | 'm' => {
                chars.next();
                let x = parse_coordinate_with_comma(&mut chars);
                let y = parse_coordinate_with_comma(&mut chars);
                commands.push(PathCommand::MoveTo { x, y });
            }
            'L' | 'l' => {
                chars.next();
                let x = parse_coordinate_with_comma(&mut chars);
                let y = parse_coordinate_with_comma(&mut chars);
                commands.push(PathCommand::LineTo { x, y });
            }
            'C' | 'c' => {
                chars.next();
                let x1 = parse_coordinate_with_comma(&mut chars);
                let y1 = parse_coordinate_with_comma(&mut chars);
                let x2 = parse_coordinate_with_comma(&mut chars);
                let y2 = parse_coordinate_with_comma(&mut chars);
                let x = parse_coordinate_with_comma(&mut chars);
                let y = parse_coordinate_with_comma(&mut chars);
                commands.push(PathCommand::CubicBezier { x1, y1, x2, y2, x, y });
            }
            'S' | 's' => {
                chars.next();
                let x2 = parse_coordinate_with_comma(&mut chars);
                let y2 = parse_coordinate_with_comma(&mut chars);
                let x = parse_coordinate_with_comma(&mut chars);
                let y = parse_coordinate_with_comma(&mut chars);
                commands.push(PathCommand::SmoothCubicBezier { x2, y2, x, y });
            }
            'Z' | 'z' => {
                chars.next();
                commands.push(PathCommand::ClosePath);
            }
            _ => {
                chars.next();
            }
        }
    }

    commands
}

fn parse_coordinate_with_comma(chars: &mut std::iter::Peekable<Chars>) -> f64 {
    while chars.peek().map_or(false, |&c| c.is_whitespace() || c == ',') {
        chars.next();
    }

    let mut coord_str = String::new();

    if chars.peek().map_or(false, |&c| c == '-' || c == '+') {
        coord_str.push(chars.next().unwrap());
    }

    while chars.peek().map_or(false, |&c| c.is_numeric() || c == '.') {
        coord_str.push(chars.next().unwrap());
    }

    coord_str.parse().unwrap_or(0.0)
}

fn flatten_path(path_data: &str, resolution: f64) -> Vec<(f64, f64)> {
    let commands = parse_path(path_data);
    let mut flattener = PathFlattener::new(resolution);

    commands.iter()
        .flat_map(|cmd| flattener.flatten_command(cmd))
        .collect()
}

fn write_points_to_file(points: Vec<(f64, f64)>, filename: &str) -> io::Result<()> {
    let path = Path::new(filename);
    let full_file_name = path.join(".h");
    File::create(&full_file_name)?;
    let mut file = File::options().read(true).write(true).open(&full_file_name)?;

    writeln!(file, "#pragma once")?;
    writeln!(file, "")?;
    writeln!(file, "#include <vector>")?;
    writeln!(file, "#include <imgui.h")?;

    writeln!(file, "")?;

    writeln!(file, "inline void Get{}Svg(const float scale = 1.0f, const ImVec2 offset = {{0.0f,0.0f}}) {{", filename)?;

    writeln!(file, "")?;

    writeln!(file, "const std::vector<unsigned int> active_index;")?;

    writeln!(file, "")?;

    for (index, window) in points.windows(2).enumerate() {
        if let [point0, point1] = window {
            writeln!(
                file,
                "DrawLogoLine({{ {:.2}, {:.2} }}, {{ {:.2}, {:.2} }}, scale, offset, 1.0f, {}, active_index);",
                point0.0,
                point0.1,
                point1.0,
                point1.1,
                index
            )?;
        }
    }

    writeln!(file, "}}")?;

    Ok(())
}

fn convert_relative_to_absolute(path_d: &str) -> String {
    let mut current_x = 0.0;
    let mut current_y = 0.0;

    let mut abs_d = String::new();

    let commands = path_d.split_whitespace();

    for command in commands {
        let mut chars = command.chars();
        let command_type = chars.next().unwrap();

        match command_type {
            'm' | 'M' => {
                let (x, y) = parse_coords(&mut chars);
                if command_type == 'm' {
                    current_x += x;
                    current_y += y;
                } else {
                    current_x = x;
                    current_y = y;
                }
                abs_d.push_str(&format!("M{},{} ", current_x, current_y));
            }
            'l' | 'L' => {
                let (x, y) = parse_coords(&mut chars);
                if command_type == 'l' {
                    current_x += x;
                    current_y += y;
                } else {
                    current_x = x;
                    current_y = y;
                }
                abs_d.push_str(&format!("L{},{} ", current_x, current_y));
            }
            'c' | 'C' => {
                let (x1, y1, x2, y2, x, y) = parse_cubic_bezier(&mut chars);
                if command_type == 'c' {
                    current_x += x;
                    current_y += y;
                } else {
                    current_x = x;
                    current_y = y;
                }
                abs_d.push_str(&format!("C{},{} {},{} {},{} ", x1, y1, x2, y2, current_x, current_y));
            }
            'z' | 'Z' => {
                abs_d.push_str("Z ");
                current_x = 0.0;
                current_y = 0.0;
            }
            _ => continue,
        }
    }

    abs_d.trim().to_string()
}

fn parse_coords(chars: &mut Chars) -> (f64, f64) {
    let x: f64 = chars.by_ref().take_while(|&c| c != ',').collect::<String>().parse().unwrap();
    let y: f64 = chars.collect::<String>().parse().unwrap();
    (x, y)
}

fn parse_cubic_bezier(chars: &mut Chars) -> (f64, f64, f64, f64, f64, f64) {
    let x1: f64 = chars.by_ref().take_while(|&c| c != ',').collect::<String>().parse().unwrap();
    let y1: f64 = chars.by_ref().take_while(|&c| c != ',').collect::<String>().parse().unwrap();
    let x2: f64 = chars.by_ref().take_while(|&c| c != ',').collect::<String>().parse().unwrap();
    let y2: f64 = chars.by_ref().take_while(|&c| c != ',').collect::<String>().parse().unwrap();
    let x: f64 = chars.by_ref().take_while(|&c| c != ',').collect::<String>().parse().unwrap();
    let y: f64 = chars.collect::<String>().parse().unwrap();
    (x1, y1, x2, y2, x, y)
}


fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: <program> <input_file> <output_file>");
        std::process::exit(1);
    }

    let input_filename = &args[1];
    let output_filename = &args[2];

    let resolution = if args.len() > 3 {
        args[3].parse::<f64>().unwrap_or(0.1)
    } else {
        0.1
    };

    let mut file = File::open(input_filename)?;
    let mut content = String::new();
    file.read_to_string(&mut content)?;

    if let Some(first_char) = content.chars().next() {
        match first_char {
            'M' | 'm' => {
                println!("Found a valid path, continuing...");
                let converted_path = convert_relative_to_absolute(&content);
                let flattened_path = flatten_path(&converted_path, resolution);
                write_points_to_file(flattened_path, output_filename)?;
            }
            _ => {
                eprintln!("Invalid input! Only provide the contents of d='<you want this in your file>'");
                std::process::exit(1);
            }
        }
    }

    Ok(())
}

